#include "opcua/binary/client/opcua_binary_client_channel.h"

#include "base/executor_conversions.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "opcua/binary/client/opcua_binary_client_secure_channel.h"
#include "opcua/binary/client/opcua_binary_client_transport.h"
#include "opcua/binary/opcua_binary_secure_channel.h"
#include "opcua/binary/opcua_binary_service_codec.h"
#include "transport/transport.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace opcua {
namespace {

struct ScriptedState {
  std::deque<std::string> incoming;
  std::vector<std::string> writes;
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

std::vector<char> BuildOpenResponseFrame(std::uint32_t channel_id,
                                         std::uint32_t token_id) {
  const OpcUaBinaryOpenSecureChannelResponse response{
      .response_header = {.request_handle = 1,
                          .service_result = scada::StatusCode::Good},
      .server_protocol_version = 0,
      .security_token = {.channel_id = channel_id,
                         .token_id = token_id,
                         .created_at = 0,
                         .revised_lifetime = 60000},
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
      .sequence_header = {.sequence_number = 1, .request_id = 1},
      .body = EncodeOpenSecureChannelResponseBody(response),
  };
  return EncodeSecureConversationMessage(message);
}

// Wraps a service-response body (produced by EncodeOpcUaBinaryServiceResponse)
// into a SecureMessage frame at the given channel/token/request_id.
std::vector<char> BuildServiceResponseFrame(std::uint32_t channel_id,
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

class OpcUaBinaryClientChannelTest : public ::testing::Test {
 protected:
  static constexpr std::uint32_t kChannelId = 42;
  static constexpr std::uint32_t kTokenId = 1;

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

  void PrimeAcknowledge(const std::shared_ptr<ScriptedState>& state) {
    state->incoming.push_back(AsString(EncodeBinaryAcknowledgeMessage(
        {.receive_buffer_size = 65535, .send_buffer_size = 65535})));
  }

  // Drives transport.Connect + secure_channel.Open, leaving the channel in
  // the opened state with the channel_id/token_id above. The test should
  // push service response frames into state->incoming AFTER calling this.
  void OpenChannel(const std::shared_ptr<ScriptedState>& state,
                   OpcUaBinaryClientTransport& transport,
                   OpcUaBinaryClientSecureChannel& secure_channel) {
    PrimeAcknowledge(state);
    state->incoming.push_back(
        AsString(BuildOpenResponseFrame(kChannelId, kTokenId)));
    ASSERT_TRUE(WaitAwaitable(executor_, transport.Connect()).good());
    ASSERT_TRUE(WaitAwaitable(executor_, secure_channel.Open()).good());
  }

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  const transport::executor any_executor_ = MakeTestAnyExecutor(executor_);
};

TEST_F(OpcUaBinaryClientChannelTest, RequestHandlesAreMonotonic) {
  auto state = std::make_shared<ScriptedState>();
  PrimeAcknowledge(state);
  auto transport = MakeClientTransport(state);
  ASSERT_TRUE(WaitAwaitable(executor_, transport->Connect()).good());
  OpcUaBinaryClientSecureChannel secure_channel{*transport};
  OpcUaBinaryClientChannel channel{{.transport = *transport,
                                    .secure_channel = secure_channel}};
  EXPECT_EQ(channel.NextRequestHandle(), 1u);
  EXPECT_EQ(channel.NextRequestHandle(), 2u);
}

TEST_F(OpcUaBinaryClientChannelTest, CallReadReturnsTypedResponse) {
  auto state = std::make_shared<ScriptedState>();
  auto transport = MakeClientTransport(state);
  OpcUaBinaryClientSecureChannel secure_channel{*transport};
  OpenChannel(state, *transport, secure_channel);

  const std::uint32_t request_handle = 7;
  // The server will see the client's secure-channel request_id == 2 (1 was
  // consumed by OPN). Queue a Read response against request_id 2.
  ReadResponse server_reply{
      .status = scada::StatusCode::Good,
      .results = {scada::DataValue{scada::Variant{std::int32_t{42}}, {}, {}, {}}},
  };
  const auto encoded_body = EncodeOpcUaBinaryServiceResponse(
      request_handle, OpcUaResponseBody{server_reply});
  ASSERT_TRUE(encoded_body.has_value());
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      kChannelId, kTokenId, /*request_id=*/2, *encoded_body)));

  OpcUaBinaryClientChannel channel{{.transport = *transport,
                                    .secure_channel = secure_channel}};
  const auto result = WaitAwaitable(
      executor_,
      channel.Call(request_handle,
                   OpcUaRequestBody{ReadRequest{.inputs = {}}}));
  ASSERT_TRUE(result.ok());
  const auto* typed = std::get_if<ReadResponse>(&result.value());
  ASSERT_NE(typed, nullptr);
  EXPECT_TRUE(typed->status.good());
  ASSERT_EQ(typed->results.size(), 1u);
  EXPECT_EQ(typed->results[0].value,
            (scada::Variant{std::int32_t{42}}));
}

TEST_F(OpcUaBinaryClientChannelTest, CallWriteReturnsStatusCodes) {
  auto state = std::make_shared<ScriptedState>();
  auto transport = MakeClientTransport(state);
  OpcUaBinaryClientSecureChannel secure_channel{*transport};
  OpenChannel(state, *transport, secure_channel);

  const std::uint32_t request_handle = 8;
  WriteResponse server_reply{
      .status = scada::StatusCode::Good,
      .results = {scada::StatusCode::Good,
                  scada::StatusCode::Bad_WrongAttributeId},
  };
  const auto encoded_body = EncodeOpcUaBinaryServiceResponse(
      request_handle, OpcUaResponseBody{server_reply});
  ASSERT_TRUE(encoded_body.has_value());
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      kChannelId, kTokenId, 2, *encoded_body)));

  OpcUaBinaryClientChannel channel{{.transport = *transport,
                                    .secure_channel = secure_channel}};
  const auto result = WaitAwaitable(
      executor_,
      channel.Call(request_handle,
                   OpcUaRequestBody{WriteRequest{.inputs = {}}}));
  ASSERT_TRUE(result.ok());
  const auto* typed = std::get_if<WriteResponse>(&result.value());
  ASSERT_NE(typed, nullptr);
  ASSERT_EQ(typed->results.size(), 2u);
  EXPECT_EQ(typed->results[0], scada::StatusCode::Good);
  EXPECT_EQ(typed->results[1], scada::StatusCode::Bad_WrongAttributeId);
}

TEST_F(OpcUaBinaryClientChannelTest, CallRejectsMismatchedRequestHandle) {
  auto state = std::make_shared<ScriptedState>();
  auto transport = MakeClientTransport(state);
  OpcUaBinaryClientSecureChannel secure_channel{*transport};
  OpenChannel(state, *transport, secure_channel);

  // Server response encoded with request_handle=999 while the client sends
  // request_handle=7.
  WriteResponse stray{.status = scada::StatusCode::Good};
  const auto encoded_body = EncodeOpcUaBinaryServiceResponse(
      999, OpcUaResponseBody{stray});
  ASSERT_TRUE(encoded_body.has_value());
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      kChannelId, kTokenId, 2, *encoded_body)));

  OpcUaBinaryClientChannel channel{{.transport = *transport,
                                    .secure_channel = secure_channel}};
  const auto result = WaitAwaitable(
      executor_,
      channel.Call(/*request_handle=*/7,
                   OpcUaRequestBody{WriteRequest{.inputs = {}}}));
  EXPECT_FALSE(result.ok());
}

TEST_F(OpcUaBinaryClientChannelTest, CallReturnsBadOnConnectionClosed) {
  auto state = std::make_shared<ScriptedState>();
  auto transport = MakeClientTransport(state);
  OpcUaBinaryClientSecureChannel secure_channel{*transport};
  OpenChannel(state, *transport, secure_channel);
  // No service response queued — the ReadFrame will see EOF and report bad.

  OpcUaBinaryClientChannel channel{{.transport = *transport,
                                    .secure_channel = secure_channel}};
  const auto result = WaitAwaitable(
      executor_,
      channel.Call(
          1, OpcUaRequestBody{ReadRequest{.inputs = {}}}));
  EXPECT_FALSE(result.ok());
  EXPECT_TRUE(result.status().bad());
}

TEST_F(OpcUaBinaryClientChannelTest,
       CallIncludesAuthenticationTokenInRequestHeader) {
  auto state = std::make_shared<ScriptedState>();
  auto transport = MakeClientTransport(state);
  OpcUaBinaryClientSecureChannel secure_channel{*transport};
  OpenChannel(state, *transport, secure_channel);

  // Queue any response so the Call completes.
  const std::uint32_t request_handle = 11;
  const auto encoded_body = EncodeOpcUaBinaryServiceResponse(
      request_handle,
      OpcUaResponseBody{OpcUaCloseSessionResponse{.status = scada::StatusCode::Good}});
  ASSERT_TRUE(encoded_body.has_value());
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      kChannelId, kTokenId, 2, *encoded_body)));

  OpcUaBinaryClientChannel channel{{.transport = *transport,
                                    .secure_channel = secure_channel}};
  channel.set_authentication_token(scada::NodeId{0xABCDEF});
  EXPECT_EQ(channel.authentication_token(), scada::NodeId{0xABCDEF});

  [[maybe_unused]] auto unused = WaitAwaitable(
      executor_, channel.Call(request_handle,
                              OpcUaRequestBody{OpcUaCloseSessionRequest{}}));

  // The outgoing SecureMessage is writes[2] (after Hello + OPN). Its body
  // starts with the OPC UA request header, whose first field is the
  // authentication_token NodeId. Verify by decoding and reading the
  // request header directly.
  ASSERT_GE(state->writes.size(), 3u);
  const auto bytes =
      std::vector<char>{state->writes[2].begin(), state->writes[2].end()};
  const auto decoded = DecodeSecureConversationMessage(bytes);
  ASSERT_TRUE(decoded.has_value());
  const auto request =
      DecodeOpcUaBinaryServiceRequest(decoded->body);
  ASSERT_TRUE(request.has_value());
  EXPECT_EQ(request->header.authentication_token, scada::NodeId{0xABCDEF});
  EXPECT_EQ(request->header.request_handle, request_handle);
}

TEST_F(OpcUaBinaryClientChannelTest,
       SplitSendReceiveBuffersOutOfOrderResponses) {
  auto state = std::make_shared<ScriptedState>();
  auto transport = MakeClientTransport(state);
  OpcUaBinaryClientSecureChannel secure_channel{*transport};
  OpenChannel(state, *transport, secure_channel);

  OpcUaBinaryClientChannel channel{{.transport = *transport,
                                    .secure_channel = secure_channel}};

  const std::uint32_t first_handle = 21;
  const std::uint32_t second_handle = 22;
  auto first_request_id = WaitAwaitable(
      executor_,
      channel.Send(first_handle, OpcUaRequestBody{ReadRequest{.inputs = {}}}));
  auto second_request_id = WaitAwaitable(
      executor_,
      channel.Send(second_handle, OpcUaRequestBody{WriteRequest{.inputs = {}}}));
  ASSERT_TRUE(first_request_id.ok());
  ASSERT_TRUE(second_request_id.ok());
  EXPECT_EQ(*first_request_id, 2u);
  EXPECT_EQ(*second_request_id, 3u);

  const auto second_body = EncodeOpcUaBinaryServiceResponse(
      second_handle,
      OpcUaResponseBody{WriteResponse{.status = scada::StatusCode::Good,
                                      .results = {scada::StatusCode::Good}}});
  const auto first_body = EncodeOpcUaBinaryServiceResponse(
      first_handle,
      OpcUaResponseBody{ReadResponse{
          .status = scada::StatusCode::Good,
          .results = {scada::DataValue{scada::Variant{std::int32_t{7}}, {}, {},
                                       {}}}}});
  ASSERT_TRUE(second_body.has_value());
  ASSERT_TRUE(first_body.has_value());
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      kChannelId, kTokenId, *second_request_id, *second_body)));
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      kChannelId, kTokenId, *first_request_id, *first_body)));

  const auto first = WaitAwaitable(
      executor_, channel.Receive(*first_request_id, first_handle));
  ASSERT_TRUE(first.ok());
  const auto* read = std::get_if<ReadResponse>(&first.value());
  ASSERT_NE(read, nullptr);
  ASSERT_EQ(read->results.size(), 1u);
  EXPECT_EQ(read->results[0].value, (scada::Variant{std::int32_t{7}}));

  const auto second = WaitAwaitable(
      executor_, channel.Receive(*second_request_id, second_handle));
  ASSERT_TRUE(second.ok());
  const auto* write = std::get_if<WriteResponse>(&second.value());
  ASSERT_NE(write, nullptr);
  ASSERT_EQ(write->results.size(), 1u);
  EXPECT_EQ(write->results[0], scada::StatusCode::Good);
}

}  // namespace
}  // namespace opcua
