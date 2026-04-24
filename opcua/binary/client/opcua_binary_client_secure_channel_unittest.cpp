#include "opcua/binary/client/opcua_binary_client_secure_channel.h"

#include "base/executor_conversions.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "opcua/binary/client/opcua_binary_client_transport.h"
#include "opcua/binary/opcua_binary_secure_channel.h"
#include "transport/transport.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace opcua {
namespace {

// Scripted scripted transport: reads drain a pre-queued deque, writes go into
// a separate deque that the test can inspect. A write-only duplex — the test
// pre-computes every server-side frame the client will see.
struct ScriptedState {
  std::deque<std::string> incoming;  // server->client bytes, pre-queued.
  std::vector<std::string> writes;   // client->server bytes captured here.
  bool opened = false;
  bool closed = false;
};

class ScriptedTransport {
 public:
  ScriptedTransport(transport::executor executor,
                    std::shared_ptr<ScriptedState> state)
      : executor_{std::move(executor)}, state_{std::move(state)} {}
  ScriptedTransport(ScriptedTransport&&) = default;
  ScriptedTransport& operator=(ScriptedTransport&&) = default;
  ScriptedTransport(const ScriptedTransport&) = delete;
  ScriptedTransport& operator=(const ScriptedTransport&) = delete;

  transport::awaitable<transport::error_code> open() {
    state_->opened = true;
    co_return transport::OK;
  }
  transport::awaitable<transport::error_code> close() {
    state_->closed = true;
    co_return transport::OK;
  }
  transport::awaitable<transport::expected<transport::any_transport>> accept() {
    co_return transport::ERR_NOT_IMPLEMENTED;
  }
  transport::awaitable<transport::expected<size_t>> read(std::span<char> data) {
    if (state_->incoming.empty()) {
      co_return size_t{0};
    }
    auto chunk = std::move(state_->incoming.front());
    state_->incoming.pop_front();
    if (chunk.size() > data.size()) {
      co_return transport::ERR_INVALID_ARGUMENT;
    }
    std::ranges::copy(chunk, data.begin());
    co_return chunk.size();
  }
  transport::awaitable<transport::expected<size_t>> write(
      std::span<const char> data) {
    state_->writes.emplace_back(data.begin(), data.end());
    co_return data.size();
  }
  std::string name() const { return "ScriptedTransport"; }
  bool message_oriented() const { return false; }
  bool connected() const { return state_->opened && !state_->closed; }
  bool active() const { return true; }
  transport::executor get_executor() { return executor_; }

 private:
  transport::executor executor_;
  std::shared_ptr<ScriptedState> state_;
};

std::string AsString(const std::vector<char>& bytes) {
  return {bytes.begin(), bytes.end()};
}

// Synthesize the OPN response frame the server would produce for the given
// channel/token, using the same encoder the server itself uses. The client
// doesn't validate the inbound request_id against anything, so we pick 1.
std::vector<char> BuildOpenResponseFrame(std::uint32_t channel_id,
                                         std::uint32_t token_id,
                                         std::uint32_t request_id,
                                         std::uint32_t request_handle,
                                         std::uint32_t revised_lifetime_ms) {
  const OpcUaBinaryOpenSecureChannelResponse response{
      .response_header = {.request_handle = request_handle,
                          .service_result = scada::StatusCode::Good},
      .server_protocol_version = 0,
      .security_token = {.channel_id = channel_id,
                         .token_id = token_id,
                         .created_at = 0,
                         .revised_lifetime = revised_lifetime_ms},
      .server_nonce = {},
  };
  const OpcUaBinarySecureConversationMessage message{
      .frame_header = {.message_type = OpcUaBinaryMessageType::SecureOpen,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = channel_id,
      .asymmetric_security_header = OpcUaBinaryAsymmetricSecurityHeader{
          .security_policy_uri = std::string{kSecurityPolicyNone},
          .sender_certificate = {},
          .receiver_certificate_thumbprint = {},
      },
      .sequence_header = {.sequence_number = 1, .request_id = request_id},
      .body = EncodeOpenSecureChannelResponseBody(response),
  };
  return EncodeSecureConversationMessage(message);
}

// Synthesize a symmetric SecureMessage frame echoing `body` back on the
// client's channel. Uses the same encoder as the real server for parity.
std::vector<char> BuildSymmetricMessageFrame(std::uint32_t channel_id,
                                             std::uint32_t token_id,
                                             std::uint32_t request_id,
                                             std::vector<char> body) {
  const OpcUaBinarySecureConversationMessage message{
      .frame_header = {.message_type = OpcUaBinaryMessageType::SecureMessage,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = channel_id,
      .asymmetric_security_header = std::nullopt,
      .symmetric_security_header =
          OpcUaBinarySymmetricSecurityHeader{.token_id = token_id},
      .sequence_header = {.sequence_number = 2, .request_id = request_id},
      .body = std::move(body),
  };
  return EncodeSecureConversationMessage(message);
}

class OpcUaBinaryClientSecureChannelTest : public ::testing::Test {
 protected:
  std::unique_ptr<OpcUaBinaryClientTransport> MakeClientTransport(
      const std::shared_ptr<ScriptedState>& state) {
    return std::make_unique<OpcUaBinaryClientTransport>(
        OpcUaBinaryClientTransportContext{
            .transport = transport::any_transport{
                ScriptedTransport{any_executor_, state}},
            .endpoint_url = "opc.tcp://localhost:4840",
            .limits = {},
        });
  }

  // Primes the ACK frame the transport expects during its Connect() call.
  void PrimeAcknowledge(const std::shared_ptr<ScriptedState>& state) {
    state->incoming.push_back(AsString(EncodeBinaryAcknowledgeMessage(
        {.receive_buffer_size = 65535, .send_buffer_size = 65535})));
  }

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  const transport::executor any_executor_ = MakeTestAnyExecutor(executor_);
};

TEST_F(OpcUaBinaryClientSecureChannelTest, OpenCapturesServerTokenAndChannel) {
  constexpr std::uint32_t kChannelId = 123;
  constexpr std::uint32_t kTokenId = 1;
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  state->incoming.push_back(AsString(BuildOpenResponseFrame(
      kChannelId, kTokenId, /*request_id=*/1, /*request_handle=*/1, 60000)));

  auto client_transport = MakeClientTransport(state);
  ASSERT_TRUE(
      WaitAwaitable(executor_, client_transport->Connect()).good());

  OpcUaBinaryClientSecureChannel client{*client_transport};
  const auto status = WaitAwaitable(executor_, client.Open());
  EXPECT_TRUE(status.good());
  EXPECT_TRUE(client.opened());
  EXPECT_EQ(client.channel_id(), kChannelId);
  EXPECT_EQ(client.token_id(), kTokenId);

  // The client wrote exactly one OPN frame to the transport. Decode it and
  // confirm the wire bytes match the OPC UA Part 6 shape.
  ASSERT_EQ(state->writes.size(), 2u);  // 0: Hello, 1: OPN
  const auto opn_bytes = std::vector<char>{state->writes[1].begin(),
                                           state->writes[1].end()};
  const auto decoded = DecodeSecureConversationMessage(opn_bytes);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->frame_header.message_type,
            OpcUaBinaryMessageType::SecureOpen);
  ASSERT_TRUE(decoded->asymmetric_security_header.has_value());
  EXPECT_EQ(decoded->asymmetric_security_header->security_policy_uri,
            kSecurityPolicyNone);
}

TEST_F(OpcUaBinaryClientSecureChannelTest, OpenPropagatesServerBadStatus) {
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  // Server says bad — the client should surface that.
  const OpcUaBinaryOpenSecureChannelResponse bad_response{
      .response_header = {.request_handle = 1,
                          .service_result = scada::StatusCode::Bad},
      .server_protocol_version = 0,
      .security_token = {},
      .server_nonce = {},
  };
  const OpcUaBinarySecureConversationMessage message{
      .frame_header = {.message_type = OpcUaBinaryMessageType::SecureOpen,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = 0,
      .asymmetric_security_header = OpcUaBinaryAsymmetricSecurityHeader{
          .security_policy_uri = std::string{kSecurityPolicyNone},
          .sender_certificate = {},
          .receiver_certificate_thumbprint = {},
      },
      .sequence_header = {.sequence_number = 1, .request_id = 1},
      .body = EncodeOpenSecureChannelResponseBody(bad_response),
  };
  state->incoming.push_back(AsString(EncodeSecureConversationMessage(message)));

  auto client_transport = MakeClientTransport(state);
  ASSERT_TRUE(
      WaitAwaitable(executor_, client_transport->Connect()).good());
  OpcUaBinaryClientSecureChannel client{*client_transport};
  const auto status = WaitAwaitable(executor_, client.Open());
  EXPECT_TRUE(status.bad());
  EXPECT_FALSE(client.opened());
}

TEST_F(OpcUaBinaryClientSecureChannelTest,
       SendServiceRequestWritesSymmetricFrame) {
  constexpr std::uint32_t kChannelId = 77;
  constexpr std::uint32_t kTokenId = 1;
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  state->incoming.push_back(AsString(BuildOpenResponseFrame(
      kChannelId, kTokenId, 1, 1, 60000)));

  auto client_transport = MakeClientTransport(state);
  ASSERT_TRUE(
      WaitAwaitable(executor_, client_transport->Connect()).good());
  OpcUaBinaryClientSecureChannel client{*client_transport};
  ASSERT_TRUE(WaitAwaitable(executor_, client.Open()).good());

  const std::vector<char> payload{'h', 'i'};
  const std::uint32_t request_id = client.NextRequestId();
  const auto send_status = WaitAwaitable(
      executor_, client.SendServiceRequest(request_id, payload));
  EXPECT_TRUE(send_status.good());

  // writes: [0]=Hello, [1]=OPN, [2]=SecureMessage with payload.
  ASSERT_EQ(state->writes.size(), 3u);
  const auto bytes =
      std::vector<char>{state->writes[2].begin(), state->writes[2].end()};
  const auto decoded = DecodeSecureConversationMessage(bytes);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->frame_header.message_type,
            OpcUaBinaryMessageType::SecureMessage);
  EXPECT_EQ(decoded->secure_channel_id, kChannelId);
  ASSERT_TRUE(decoded->symmetric_security_header.has_value());
  EXPECT_EQ(decoded->symmetric_security_header->token_id, kTokenId);
  EXPECT_EQ(decoded->sequence_header.request_id, request_id);
  EXPECT_EQ(decoded->body, payload);
}

TEST_F(OpcUaBinaryClientSecureChannelTest, ReadServiceResponseReturnsBody) {
  constexpr std::uint32_t kChannelId = 77;
  constexpr std::uint32_t kTokenId = 1;
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  state->incoming.push_back(AsString(BuildOpenResponseFrame(
      kChannelId, kTokenId, 1, 1, 60000)));
  const std::vector<char> expected_body{'p', 'a', 'y', '!'};
  state->incoming.push_back(AsString(BuildSymmetricMessageFrame(
      kChannelId, kTokenId, /*request_id=*/42, expected_body)));

  auto client_transport = MakeClientTransport(state);
  ASSERT_TRUE(
      WaitAwaitable(executor_, client_transport->Connect()).good());
  OpcUaBinaryClientSecureChannel client{*client_transport};
  ASSERT_TRUE(WaitAwaitable(executor_, client.Open()).good());

  const auto response = WaitAwaitable(executor_, client.ReadServiceResponse());
  ASSERT_TRUE(response.ok());
  EXPECT_EQ(response->request_id, 42u);
  EXPECT_EQ(response->body, expected_body);
}

TEST_F(OpcUaBinaryClientSecureChannelTest,
       ReadServiceResponseRejectsWrongTokenId) {
  constexpr std::uint32_t kChannelId = 77;
  constexpr std::uint32_t kTokenId = 1;
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  state->incoming.push_back(AsString(BuildOpenResponseFrame(
      kChannelId, kTokenId, 1, 1, 60000)));
  // Symmetric frame on a different token_id — client must reject.
  state->incoming.push_back(AsString(BuildSymmetricMessageFrame(
      kChannelId, /*token_id=*/99, 1, std::vector<char>{'x'})));

  auto client_transport = MakeClientTransport(state);
  ASSERT_TRUE(
      WaitAwaitable(executor_, client_transport->Connect()).good());
  OpcUaBinaryClientSecureChannel client{*client_transport};
  ASSERT_TRUE(WaitAwaitable(executor_, client.Open()).good());

  const auto response = WaitAwaitable(executor_, client.ReadServiceResponse());
  EXPECT_FALSE(response.ok());
}

TEST_F(OpcUaBinaryClientSecureChannelTest, CloseWritesCloseSecureChannelFrame) {
  constexpr std::uint32_t kChannelId = 77;
  constexpr std::uint32_t kTokenId = 1;
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  state->incoming.push_back(AsString(BuildOpenResponseFrame(
      kChannelId, kTokenId, 1, 1, 60000)));

  auto client_transport = MakeClientTransport(state);
  ASSERT_TRUE(
      WaitAwaitable(executor_, client_transport->Connect()).good());
  OpcUaBinaryClientSecureChannel client{*client_transport};
  ASSERT_TRUE(WaitAwaitable(executor_, client.Open()).good());

  const auto close_status = WaitAwaitable(executor_, client.Close());
  EXPECT_TRUE(close_status.good());
  EXPECT_FALSE(client.opened());

  // [0]=Hello, [1]=OPN, [2]=CLO.
  ASSERT_EQ(state->writes.size(), 3u);
  const auto bytes =
      std::vector<char>{state->writes[2].begin(), state->writes[2].end()};
  const auto decoded = DecodeSecureConversationMessage(bytes);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->frame_header.message_type,
            OpcUaBinaryMessageType::SecureClose);
}

TEST_F(OpcUaBinaryClientSecureChannelTest, CloseWithoutOpenIsNoOp) {
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  auto client_transport = MakeClientTransport(state);
  ASSERT_TRUE(
      WaitAwaitable(executor_, client_transport->Connect()).good());
  OpcUaBinaryClientSecureChannel client{*client_transport};

  const auto status = WaitAwaitable(executor_, client.Close());
  EXPECT_TRUE(status.good());
  // No extra frame beyond the Hello was written.
  EXPECT_EQ(state->writes.size(), 1u);
}

TEST_F(OpcUaBinaryClientSecureChannelTest, SendBeforeOpenReturnsBad) {
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  auto client_transport = MakeClientTransport(state);
  ASSERT_TRUE(
      WaitAwaitable(executor_, client_transport->Connect()).good());
  OpcUaBinaryClientSecureChannel client{*client_transport};

  const auto status = WaitAwaitable(
      executor_, client.SendServiceRequest(1, std::vector<char>{'x'}));
  EXPECT_TRUE(status.bad());
}

TEST_F(OpcUaBinaryClientSecureChannelTest, RequestIdsAreMonotonic) {
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  auto client_transport = MakeClientTransport(state);
  ASSERT_TRUE(
      WaitAwaitable(executor_, client_transport->Connect()).good());
  OpcUaBinaryClientSecureChannel client{*client_transport};

  EXPECT_EQ(client.NextRequestId(), 1u);
  EXPECT_EQ(client.NextRequestId(), 2u);
  EXPECT_EQ(client.NextRequestId(), 3u);
}

}  // namespace
}  // namespace opcua
