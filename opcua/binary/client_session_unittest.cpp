#include "opcua/client_protocol_session.h"

#include "base/executor_conversions.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "opcua/client_channel.h"
#include "opcua/binary/client_connection.h"
#include "opcua/binary/client_secure_channel.h"
#include "opcua/binary/client_transport.h"
#include "opcua/binary/secure_channel.h"
#include "opcua/binary/service_codec.h"
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

// --- Response frame builders (each mirrors what a live server would send
// for the given client-visible request_id) -----------------------------

constexpr std::uint32_t kChannelId = 42;
constexpr std::uint32_t kTokenId = 1;

std::vector<char> BuildOpenResponseFrame() {
  const OpcUaBinaryOpenSecureChannelResponse response{
      .response_header = {.request_handle = 1,
                          .service_result = scada::StatusCode::Good},
      .server_protocol_version = 0,
      .security_token = {.channel_id = kChannelId,
                         .token_id = kTokenId,
                         .created_at = 0,
                         .revised_lifetime = 60000},
      .server_nonce = {},
  };
  const OpcUaBinarySecureConversationMessage message{
      .frame_header = {.message_type = OpcUaBinaryMessageType::SecureOpen,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = kChannelId,
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

std::vector<char> BuildServiceResponseFrame(std::uint32_t request_id,
                                            std::uint32_t request_handle,
                                            OpcUaResponseBody body) {
  const auto encoded =
      EncodeOpcUaBinaryServiceResponse(request_handle, std::move(body));
  const OpcUaBinarySecureConversationMessage message{
      .frame_header = {.message_type = OpcUaBinaryMessageType::SecureMessage,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = kChannelId,
      .asymmetric_security_header = std::nullopt,
      .symmetric_security_header =
          OpcUaBinarySymmetricSecurityHeader{.token_id = kTokenId},
      .sequence_header = {.sequence_number = request_id + 1,
                          .request_id = request_id},
      .body = encoded.value(),
  };
  return EncodeSecureConversationMessage(message);
}

class OpcUaClientProtocolSessionTest : public ::testing::Test {
 protected:
  // Every client call advances the secure channel's request_id by 1. The
  // helpers below queue responses for specific request_ids so the reader
  // sees them in the expected order:
  //   request_id 1 -> OPN response
  //   request_id 2 -> first service call (CreateSession during Create())
  //   request_id 3 -> ActivateSession
  //   request_id 4+ -> post-Create service calls
  void PrimeConnectAndOpen(const std::shared_ptr<ScriptedState>& state) {
    state->incoming.push_back(AsString(EncodeBinaryAcknowledgeMessage(
        {.receive_buffer_size = 65535, .send_buffer_size = 65535})));
    state->incoming.push_back(AsString(BuildOpenResponseFrame()));
  }

  // Queues the Create + Activate responses the session needs to finish
  // Create(). Returns the authentication_token the client will see.
  scada::NodeId PrimeSessionEstablishment(
      const std::shared_ptr<ScriptedState>& state) {
    const scada::NodeId session_id{111};
    const scada::NodeId auth_token{0xABCDEF};
    state->incoming.push_back(AsString(BuildServiceResponseFrame(
        /*request_id=*/2, /*request_handle=*/1,
        OpcUaResponseBody{OpcUaCreateSessionResponse{
            .status = scada::StatusCode::Good,
            .session_id = session_id,
            .authentication_token = auth_token,
            .server_nonce = scada::ByteString{},
            .revised_timeout = base::TimeDelta::FromSeconds(60),
        }})));
    state->incoming.push_back(AsString(BuildServiceResponseFrame(
        /*request_id=*/3, /*request_handle=*/2,
        OpcUaResponseBody{OpcUaActivateSessionResponse{
            .status = scada::StatusCode::Good}})));
    return auth_token;
  }

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  const transport::executor any_executor_ = MakeTestAnyExecutor(executor_);
};

TEST_F(OpcUaClientProtocolSessionTest, CreateRunsCreateAndActivate) {
  auto state = std::make_shared<ScriptedState>();
  PrimeConnectAndOpen(state);
  const auto auth_token = PrimeSessionEstablishment(state);

  OpcUaBinaryClientTransport transport{OpcUaBinaryClientTransportContext{
      .transport = transport::any_transport{
          ScriptedTransport{any_executor_, state}},
      .endpoint_url = "opc.tcp://localhost:4840",
      .limits = {},
  }};
  OpcUaBinaryClientSecureChannel secure_channel{transport};
  OpcUaBinaryClientConnection connection{
      {.transport = transport, .secure_channel = secure_channel}};
  OpcUaClientChannel channel{{.connection = connection}};
  OpcUaClientProtocolSession session{
      {.connection = connection, .channel = channel}};

  const auto status = WaitAwaitable(executor_, session.Create());
  ASSERT_TRUE(status.good());
  EXPECT_TRUE(session.is_active());
  EXPECT_EQ(session.session_id(), scada::NodeId{111});
  EXPECT_EQ(session.authentication_token(), auth_token);
}

TEST_F(OpcUaClientProtocolSessionTest, CreatePropagatesCreateSessionBadStatus) {
  auto state = std::make_shared<ScriptedState>();
  PrimeConnectAndOpen(state);
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/2, /*request_handle=*/1,
      OpcUaResponseBody{OpcUaCreateSessionResponse{
          .status = scada::StatusCode::Bad_WrongLoginCredentials}})));

  OpcUaBinaryClientTransport transport{OpcUaBinaryClientTransportContext{
      .transport = transport::any_transport{
          ScriptedTransport{any_executor_, state}},
      .endpoint_url = "opc.tcp://localhost:4840",
      .limits = {},
  }};
  OpcUaBinaryClientSecureChannel secure_channel{transport};
  OpcUaBinaryClientConnection connection{
      {.transport = transport, .secure_channel = secure_channel}};
  OpcUaClientChannel channel{{.connection = connection}};
  OpcUaClientProtocolSession session{
      {.connection = connection, .channel = channel}};

  const auto status = WaitAwaitable(executor_, session.Create());
  EXPECT_TRUE(status.bad());
  EXPECT_FALSE(session.is_active());
}

TEST_F(OpcUaClientProtocolSessionTest, ReadReturnsDataValuesOnSuccess) {
  auto state = std::make_shared<ScriptedState>();
  PrimeConnectAndOpen(state);
  PrimeSessionEstablishment(state);
  // Read is the next call after Activate, so request_id=4, request_handle=3.
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/4, /*request_handle=*/3,
      OpcUaResponseBody{ReadResponse{
          .status = scada::StatusCode::Good,
          .results = {scada::DataValue{scada::Variant{std::int32_t{7}}, {}, {}, {}}},
      }})));

  OpcUaBinaryClientTransport transport{OpcUaBinaryClientTransportContext{
      .transport = transport::any_transport{
          ScriptedTransport{any_executor_, state}},
      .endpoint_url = "opc.tcp://localhost:4840",
      .limits = {},
  }};
  OpcUaBinaryClientSecureChannel secure_channel{transport};
  OpcUaBinaryClientConnection connection{
      {.transport = transport, .secure_channel = secure_channel}};
  OpcUaClientChannel channel{{.connection = connection}};
  OpcUaClientProtocolSession session{
      {.connection = connection, .channel = channel}};
  ASSERT_TRUE(WaitAwaitable(executor_, session.Create()).good());

  const auto read = WaitAwaitable(
      executor_, session.Read(std::vector<scada::ReadValueId>{
                     {.node_id = scada::NodeId{1},
                      .attribute_id = scada::AttributeId::Value}}));
  ASSERT_TRUE(read.ok());
  ASSERT_EQ(read->size(), 1u);
  EXPECT_EQ((*read)[0].value, (scada::Variant{std::int32_t{7}}));
}

TEST_F(OpcUaClientProtocolSessionTest, WriteReturnsStatusCodes) {
  auto state = std::make_shared<ScriptedState>();
  PrimeConnectAndOpen(state);
  PrimeSessionEstablishment(state);
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/4, /*request_handle=*/3,
      OpcUaResponseBody{WriteResponse{
          .status = scada::StatusCode::Good,
          .results = {scada::StatusCode::Good}}})));

  OpcUaBinaryClientTransport transport{OpcUaBinaryClientTransportContext{
      .transport = transport::any_transport{
          ScriptedTransport{any_executor_, state}},
      .endpoint_url = "opc.tcp://localhost:4840",
      .limits = {},
  }};
  OpcUaBinaryClientSecureChannel secure_channel{transport};
  OpcUaBinaryClientConnection connection{
      {.transport = transport, .secure_channel = secure_channel}};
  OpcUaClientChannel channel{{.connection = connection}};
  OpcUaClientProtocolSession session{
      {.connection = connection, .channel = channel}};
  ASSERT_TRUE(WaitAwaitable(executor_, session.Create()).good());

  const auto write = WaitAwaitable(
      executor_,
      session.Write(std::vector<scada::WriteValue>{
          {.node_id = scada::NodeId{1},
           .attribute_id = scada::AttributeId::Value,
           .value = scada::Variant{std::int32_t{99}}}}));
  ASSERT_TRUE(write.ok());
  ASSERT_EQ(write->size(), 1u);
  EXPECT_EQ((*write)[0], scada::StatusCode::Good);
}

TEST_F(OpcUaClientProtocolSessionTest, BrowseReturnsReferences) {
  auto state = std::make_shared<ScriptedState>();
  PrimeConnectAndOpen(state);
  PrimeSessionEstablishment(state);
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/4, /*request_handle=*/3,
      OpcUaResponseBody{BrowseResponse{
          .status = scada::StatusCode::Good,
          .results = {scada::BrowseResult{
              .status_code = scada::StatusCode::Good,
              .references = {scada::ReferenceDescription{
                  .reference_type_id = scada::NodeId{35},
                  .forward = true,
                  .node_id = scada::NodeId{1000}}}}}}})));

  OpcUaBinaryClientTransport transport{OpcUaBinaryClientTransportContext{
      .transport = transport::any_transport{
          ScriptedTransport{any_executor_, state}},
      .endpoint_url = "opc.tcp://localhost:4840",
      .limits = {},
  }};
  OpcUaBinaryClientSecureChannel secure_channel{transport};
  OpcUaBinaryClientConnection connection{
      {.transport = transport, .secure_channel = secure_channel}};
  OpcUaClientChannel channel{{.connection = connection}};
  OpcUaClientProtocolSession session{
      {.connection = connection, .channel = channel}};
  ASSERT_TRUE(WaitAwaitable(executor_, session.Create()).good());

  const auto browse = WaitAwaitable(
      executor_,
      session.Browse(std::vector<scada::BrowseDescription>{
          {.node_id = scada::NodeId{85}}}));
  ASSERT_TRUE(browse.ok());
  ASSERT_EQ(browse->size(), 1u);
  ASSERT_EQ((*browse)[0].references.size(), 1u);
  EXPECT_EQ((*browse)[0].references[0].node_id, scada::NodeId{1000});
}

TEST_F(OpcUaClientProtocolSessionTest, CallRoundTripsArguments) {
  auto state = std::make_shared<ScriptedState>();
  PrimeConnectAndOpen(state);
  PrimeSessionEstablishment(state);
  CallResponse server_reply;
  server_reply.results.push_back(
      {.status = scada::StatusCode::Good,
       .input_argument_results = {scada::StatusCode::Good},
       .output_arguments = {scada::Variant{std::int32_t{123}}}});
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/4, /*request_handle=*/3,
      OpcUaResponseBody{server_reply})));

  OpcUaBinaryClientTransport transport{OpcUaBinaryClientTransportContext{
      .transport = transport::any_transport{
          ScriptedTransport{any_executor_, state}},
      .endpoint_url = "opc.tcp://localhost:4840",
      .limits = {},
  }};
  OpcUaBinaryClientSecureChannel secure_channel{transport};
  OpcUaBinaryClientConnection connection{
      {.transport = transport, .secure_channel = secure_channel}};
  OpcUaClientChannel channel{{.connection = connection}};
  OpcUaClientProtocolSession session{
      {.connection = connection, .channel = channel}};
  ASSERT_TRUE(WaitAwaitable(executor_, session.Create()).good());

  const auto call = WaitAwaitable(
      executor_, session.Call(scada::NodeId{2000}, scada::NodeId{3000},
                              {scada::Variant{std::int32_t{5}}}));
  ASSERT_TRUE(call.ok());
  EXPECT_TRUE(call->status.good());
  ASSERT_EQ(call->output_arguments.size(), 1u);
  EXPECT_EQ(call->output_arguments[0],
            (scada::Variant{std::int32_t{123}}));
}

TEST_F(OpcUaClientProtocolSessionTest, CloseRunsCloseSessionBestEffort) {
  auto state = std::make_shared<ScriptedState>();
  PrimeConnectAndOpen(state);
  PrimeSessionEstablishment(state);
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/4, /*request_handle=*/3,
      OpcUaResponseBody{OpcUaCloseSessionResponse{
          .status = scada::StatusCode::Good}})));

  OpcUaBinaryClientTransport transport{OpcUaBinaryClientTransportContext{
      .transport = transport::any_transport{
          ScriptedTransport{any_executor_, state}},
      .endpoint_url = "opc.tcp://localhost:4840",
      .limits = {},
  }};
  OpcUaBinaryClientSecureChannel secure_channel{transport};
  OpcUaBinaryClientConnection connection{
      {.transport = transport, .secure_channel = secure_channel}};
  OpcUaClientChannel channel{{.connection = connection}};
  OpcUaClientProtocolSession session{
      {.connection = connection, .channel = channel}};
  ASSERT_TRUE(WaitAwaitable(executor_, session.Create()).good());

  const auto status = WaitAwaitable(executor_, session.Close());
  EXPECT_TRUE(status.good());
  EXPECT_FALSE(session.is_active());
}

}  // namespace
}  // namespace opcua
