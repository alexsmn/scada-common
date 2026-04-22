#include "opcua/opcua_binary_service_dispatcher.h"
#include "opcua/opcua_binary_codec_utils.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "base/time_utils.h"
#include "opcua/opcua_binary_protocol.h"
#include "opcua/opcua_binary_secure_channel.h"
#include "opcua/opcua_binary_tcp_connection.h"
#include "scada/attribute_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/node_management_service_mock.h"
#include "scada/view_service_mock.h"
#include "transport/transport.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <deque>

using namespace testing;

namespace opcua {
namespace {

constexpr std::uint32_t kCreateSessionRequestBinaryEncodingId = 461;
constexpr std::uint32_t kCreateSessionResponseBinaryEncodingId = 464;
constexpr std::uint32_t kActivateSessionRequestBinaryEncodingId = 467;
constexpr std::uint32_t kActivateSessionResponseBinaryEncodingId = 470;
constexpr std::uint32_t kReadRequestBinaryEncodingId = 629;
constexpr std::uint32_t kReadResponseBinaryEncodingId = 632;
constexpr std::uint32_t kWriteRequestBinaryEncodingId = 671;
constexpr std::uint32_t kWriteResponseBinaryEncodingId = 674;
constexpr std::uint32_t kUserNameIdentityTokenBinaryEncodingId = 324;

struct StreamPeerState {
  std::deque<std::string> incoming;
  std::vector<std::string> writes;
  bool opened = false;
  bool closed = false;
};

class ScriptedStreamTransport {
 public:
  ScriptedStreamTransport(transport::executor executor,
                          std::shared_ptr<StreamPeerState> state)
      : executor_{std::move(executor)}, state_{std::move(state)} {}
  ScriptedStreamTransport(ScriptedStreamTransport&&) = default;
  ScriptedStreamTransport& operator=(ScriptedStreamTransport&&) = default;
  ScriptedStreamTransport(const ScriptedStreamTransport&) = delete;
  ScriptedStreamTransport& operator=(const ScriptedStreamTransport&) = delete;

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

  std::string name() const { return "ScriptedStreamTransport"; }
  bool message_oriented() const { return false; }
  bool connected() const { return state_->opened && !state_->closed; }
  bool active() const { return true; }
  transport::executor get_executor() { return executor_; }

 private:
  transport::executor executor_;
  std::shared_ptr<StreamPeerState> state_;
};

scada::NodeId NumericNode(scada::NumericId id, scada::NamespaceIndex ns = 2) {
  return {id, ns};
}

base::Time ParseTime(std::string_view value) {
  base::Time result;
  EXPECT_TRUE(Deserialize(value, result));
  return result;
}

using binary::AppendByteString;
using binary::AppendDouble;
using binary::AppendInt32;
using binary::AppendInt64;
using binary::AppendMessage;
using binary::AppendNumericNodeId;
using binary::AppendUaString;
using binary::AppendUInt8;
using binary::AppendUInt16;
using binary::AppendUInt32;
using binary::ReadDouble;
using binary::ReadInt32;
using binary::ReadInt64;
using binary::ReadMessage;
using binary::ReadNumericNodeId;
using binary::ReadUInt8;
using binary::ReadUInt16;
using binary::ReadUInt32;

void AppendRequestHeader(std::vector<char>& bytes,
                         const scada::NodeId& authentication_token,
                         std::uint32_t request_handle) {
  AppendNumericNodeId(bytes, authentication_token);
  AppendInt64(bytes, 0);
  AppendUInt32(bytes, request_handle);
  AppendUInt32(bytes, 0);
  AppendUaString(bytes, "");
  AppendUInt32(bytes, 0);
  AppendNumericNodeId(bytes, scada::NodeId{});
  AppendUInt8(bytes, 0x00);
}

std::vector<char> EncodeOpenRequestBody(std::uint32_t request_handle) {
  std::vector<char> payload;
  AppendNumericNodeId(payload, scada::NodeId{});
  AppendInt64(payload, 0);
  AppendUInt32(payload, request_handle);
  AppendUInt32(payload, 0);
  AppendUaString(payload, "");
  AppendUInt32(payload, 0);
  AppendNumericNodeId(payload, scada::NodeId{});
  AppendUInt8(payload, 0x00);
  AppendUInt32(payload, 0);
  AppendUInt32(payload,
               static_cast<std::uint32_t>(
                   OpcUaBinarySecurityTokenRequestType::Issue));
  AppendUInt32(
      payload,
      static_cast<std::uint32_t>(OpcUaBinaryMessageSecurityMode::None));
  AppendByteString(payload, {});
  AppendUInt32(payload, 60000);

  std::vector<char> body;
  AppendNumericNodeId(body, scada::NodeId{kOpenSecureChannelRequestBinaryEncodingId});
  AppendUInt8(body, 0x01);
  AppendInt32(body, static_cast<std::int32_t>(payload.size()));
  body.insert(body.end(), payload.begin(), payload.end());
  return body;
}

std::vector<char> EncodeCreateSessionRequestBody(std::uint32_t request_handle,
                                                 double requested_timeout_ms) {
  std::vector<char> payload;
  AppendRequestHeader(payload, scada::NodeId{}, request_handle);
  AppendUaString(payload, "");
  AppendUaString(payload, "");
  AppendUInt8(payload, 0);
  AppendInt32(payload, 0);
  AppendUaString(payload, "");
  AppendUaString(payload, "");
  AppendInt32(payload, -1);
  AppendUaString(payload, "");
  AppendUaString(payload, "opc.tcp://localhost:4840");
  AppendUaString(payload, "binary-session");
  AppendByteString(payload, {});
  AppendByteString(payload, {});
  AppendDouble(payload, requested_timeout_ms);
  AppendUInt32(payload, 0);

  std::vector<char> body;
  AppendMessage(body, kCreateSessionRequestBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeUserNameActivateRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    std::string_view user_name,
    std::string_view password) {
  std::vector<char> payload;
  AppendRequestHeader(payload, authentication_token, request_handle);
  AppendUaString(payload, "");
  AppendByteString(payload, {});
  AppendInt32(payload, -1);
  AppendInt32(payload, -1);
  std::vector<char> identity;
  AppendUaString(identity, "");
  AppendUaString(identity, user_name);
  AppendByteString(identity, scada::ByteString{password.begin(), password.end()});
  AppendUaString(identity, "");
  AppendNumericNodeId(payload, scada::NodeId{kUserNameIdentityTokenBinaryEncodingId});
  AppendUInt8(payload, 0x01);
  AppendInt32(payload, static_cast<std::int32_t>(identity.size()));
  payload.insert(payload.end(), identity.begin(), identity.end());
  AppendUaString(payload, "");
  AppendByteString(payload, {});

  std::vector<char> body;
  AppendMessage(body, kActivateSessionRequestBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeReadRequestBody(std::uint32_t request_handle,
                                        const scada::NodeId& authentication_token,
                                        const scada::ReadValueId& read_value_id) {
  std::vector<char> payload;
  AppendRequestHeader(payload, authentication_token, request_handle);
  AppendDouble(payload, 0);
  AppendUInt32(payload, 2);  // Both
  AppendInt32(payload, 1);
  AppendNumericNodeId(payload, read_value_id.node_id);
  AppendUInt32(payload, static_cast<std::uint32_t>(read_value_id.attribute_id));
  AppendUaString(payload, "");
  AppendUInt16(payload, 0);
  AppendUaString(payload, "");

  std::vector<char> body;
  AppendMessage(body, kReadRequestBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeWriteRequestBody(std::uint32_t request_handle,
                                         const scada::NodeId& authentication_token,
                                         const scada::WriteValue& write_value) {
  std::vector<char> payload;
  AppendRequestHeader(payload, authentication_token, request_handle);
  AppendInt32(payload, 1);
  AppendNumericNodeId(payload, write_value.node_id);
  AppendUInt32(payload, static_cast<std::uint32_t>(write_value.attribute_id));
  AppendUaString(payload, "");
  AppendUInt8(payload, 0x01);
  const auto* value = write_value.value.get_if<scada::Double>();
  EXPECT_NE(value, nullptr);
  AppendUInt8(payload, 11);
  AppendDouble(payload, value == nullptr ? 0.0 : *value);

  std::vector<char> body;
  AppendMessage(body, kWriteRequestBinaryEncodingId, payload);
  return body;
}

std::string AsString(const std::vector<char>& bytes) {
  return {bytes.begin(), bytes.end()};
}

struct DecodedCreateSessionResponse {
  scada::NodeId session_id;
  scada::NodeId authentication_token;
};

std::optional<DecodedCreateSessionResponse> DecodeCreateSessionResponse(
    const std::vector<char>& payload) {
  const auto message = ReadMessage(payload);
  if (!message.has_value() ||
      message->first != kCreateSessionResponseBinaryEncodingId) {
    return std::nullopt;
  }
  const auto& body = message->second;
  std::size_t offset = 0;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  if (!ReadInt64(body, offset, ignored_timestamp) ||
      !ReadUInt32(body, offset, ignored_request_handle) ||
      !ReadUInt32(body, offset, ignored_status) ||
      !ReadUInt8(body, offset, ignored_byte) ||
      !ReadInt32(body, offset, ignored_array) ||
      !ReadNumericNodeId(body, offset, ignored_header_extension) ||
      !ReadUInt8(body, offset, ignored_byte) ||
      !ReadNumericNodeId(body, offset, session_id) ||
      !ReadNumericNodeId(body, offset, authentication_token)) {
    return std::nullopt;
  }
  return DecodedCreateSessionResponse{
      .session_id = session_id,
      .authentication_token = authentication_token,
  };
}

std::optional<double> DecodeSingleDoubleReadResponse(
    const std::vector<char>& payload) {
  const auto message = ReadMessage(payload);
  if (!message.has_value() || message->first != kReadResponseBinaryEncodingId) {
    return std::nullopt;
  }
  const auto& body = message->second;

  std::size_t offset = 0;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  if (!ReadInt64(body, offset, ignored_timestamp) ||
      !ReadUInt32(body, offset, ignored_request_handle) ||
      !ReadUInt32(body, offset, ignored_status) ||
      !ReadUInt8(body, offset, ignored_byte) ||
      !ReadInt32(body, offset, ignored_array) ||
      !ReadNumericNodeId(body, offset, ignored_header_extension) ||
      !ReadUInt8(body, offset, ignored_byte)) {
    return std::nullopt;
  }

  std::int32_t result_count = 0;
  if (!ReadInt32(body, offset, result_count) || result_count != 1) {
    return std::nullopt;
  }

  std::uint8_t data_value_mask = 0;
  if (!ReadUInt8(body, offset, data_value_mask) || (data_value_mask & 0x01) == 0) {
    return std::nullopt;
  }

  std::uint8_t variant_mask = 0;
  double value = 0;
  if (!ReadUInt8(body, offset, variant_mask) || variant_mask != 11 ||
      !ReadDouble(body, offset, value)) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::uint32_t> DecodeSingleWriteResponseStatus(
    const std::vector<char>& payload) {
  const auto message = ReadMessage(payload);
  if (!message.has_value() || message->first != kWriteResponseBinaryEncodingId) {
    return std::nullopt;
  }
  const auto& body = message->second;

  std::size_t offset = 0;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  if (!ReadInt64(body, offset, ignored_timestamp) ||
      !ReadUInt32(body, offset, ignored_request_handle) ||
      !ReadUInt32(body, offset, ignored_status) ||
      !ReadUInt8(body, offset, ignored_byte) ||
      !ReadInt32(body, offset, ignored_array) ||
      !ReadNumericNodeId(body, offset, ignored_header_extension) ||
      !ReadUInt8(body, offset, ignored_byte)) {
    return std::nullopt;
  }

  std::int32_t result_count = 0;
  std::uint32_t result_status = 0;
  if (!ReadInt32(body, offset, result_count) || result_count != 1 ||
      !ReadUInt32(body, offset, result_status)) {
    return std::nullopt;
  }
  return result_status;
}

class TestMonitoredItemService : public scada::MonitoredItemService {
 public:
  std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId&,
      const scada::MonitoringParameters&) override {
    return nullptr;
  }
};

class OpcUaBinaryServiceDispatcherTest : public ::testing::Test {
 protected:
  void RunPeer(const std::shared_ptr<StreamPeerState>& peer,
               OpcUaBinaryServiceDispatcher& dispatcher) {
    WaitAwaitable(executor_,
                  OpcUaBinaryTcpConnection{
                      {.transport = transport::any_transport{
                           ScriptedStreamTransport{any_executor_, peer}},
                       .limits = server_limits_,
                       .on_secure_frame =
                           [&dispatcher](std::vector<char> payload)
                               -> Awaitable<std::optional<std::vector<char>>> {
                         co_return co_await dispatcher.HandlePayload(
                             std::move(payload));
                       }}}
                      .Run());
  }

  base::Time now_ = ParseTime("2026-04-21 11:00:00");
  const scada::NodeId expected_user_id_ = NumericNode(700, 5);
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  const transport::executor any_executor_ = MakeTestAnyExecutor(executor_);
  const OpcUaBinaryTransportLimits server_limits_{
      .protocol_version = 0,
      .receive_buffer_size = 8192,
      .send_buffer_size = 4096,
      .max_message_size = 0,
      .max_chunk_count = 0,
  };
  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockViewService> view_service_;
  StrictMock<scada::MockHistoryService> history_service_;
  StrictMock<scada::MockMethodService> method_service_;
  StrictMock<scada::MockNodeManagementService> node_management_service_;
  TestMonitoredItemService monitored_item_service_;
  opcua_ws::OpcUaWsSessionManager session_manager_{{
      .authenticator =
          [this](scada::LocalizedText user_name,
                 scada::LocalizedText password)
              -> Awaitable<scada::StatusOr<scada::AuthenticationResult>> {
        EXPECT_EQ(user_name, scada::LocalizedText{u"operator"});
        EXPECT_EQ(password, scada::LocalizedText{u"secret"});
        co_return scada::AuthenticationResult{
            .user_id = expected_user_id_, .multi_sessions = true};
      },
      .now = [this] { return now_; },
  }};
  OpcUaBinaryConnectionState connection_;
  OpcUaBinaryRuntime runtime_{{
      .executor = executor_,
      .session_manager = session_manager_,
      .monitored_item_service = monitored_item_service_,
      .attribute_service = attribute_service_,
      .view_service = view_service_,
      .history_service = history_service_,
      .method_service = method_service_,
      .node_management_service = node_management_service_,
      .now = [this] { return now_; },
  }};
};

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesReadAfterActivatedSession) {
  OpcUaBinaryServiceDispatcher dispatcher{
      {.runtime = runtime_,
       .session_manager = session_manager_,
       .connection = connection_}};

  const auto created = WaitAwaitable(
      executor_, dispatcher.HandlePayload(EncodeCreateSessionRequestBody(1, 45000)));
  ASSERT_TRUE(created.has_value());
  const auto session = DecodeCreateSessionResponse(*created);
  ASSERT_TRUE(session.has_value());

  const auto activated = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeUserNameActivateRequestBody(
          2, session->authentication_token, "operator", "secret")));
  ASSERT_TRUE(activated.has_value());

  const scada::ReadValueId read_value_id{
      .node_id = NumericNode(12),
      .attribute_id = scada::AttributeId::Value,
  };
  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(context.user_id(), expected_user_id_);
        ASSERT_EQ(inputs->size(), 1u);
        EXPECT_EQ((*inputs)[0], read_value_id);
        callback(scada::StatusCode::Good,
                 {scada::DataValue{scada::Variant{42.5}, {}, now_, now_}});
      }));

  const auto read = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(
          EncodeReadRequestBody(3, session->authentication_token, read_value_id)));
  ASSERT_TRUE(read.has_value());
  const auto value = DecodeSingleDoubleReadResponse(*read);
  ASSERT_TRUE(value.has_value());
  EXPECT_DOUBLE_EQ(*value, 42.5);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       BootstrapsAndReadsEndToEndOverTcpConnection) {
  OpcUaBinaryServiceDispatcher dispatcher{
      {.runtime = runtime_,
       .session_manager = session_manager_,
       .connection = connection_}};

  auto peer = std::make_shared<StreamPeerState>();
  const auto hello = EncodeBinaryHelloMessage(
      {.protocol_version = 0,
       .receive_buffer_size = 16384,
       .send_buffer_size = 2048,
       .max_message_size = 0,
       .max_chunk_count = 0,
       .endpoint_url = "opc.tcp://localhost:4840"});
  const auto open = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureOpen,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 0,
       .asymmetric_security_header = OpcUaBinaryAsymmetricSecurityHeader{
           .security_policy_uri = std::string{kSecurityPolicyNone},
           .sender_certificate = {},
           .receiver_certificate_thumbprint = {},
       },
       .sequence_header = {.sequence_number = 1, .request_id = 1},
       .body = EncodeOpenRequestBody(1)});
  const auto create = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureMessage,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 1,
       .symmetric_security_header =
           OpcUaBinarySymmetricSecurityHeader{.token_id = 1},
       .sequence_header = {.sequence_number = 2, .request_id = 10},
       .body = EncodeCreateSessionRequestBody(10, 45000)});
  const auto activate = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureMessage,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 1,
       .symmetric_security_header =
           OpcUaBinarySymmetricSecurityHeader{.token_id = 1},
       .sequence_header = {.sequence_number = 3, .request_id = 11},
       .body = EncodeUserNameActivateRequestBody(
           11, NumericNode(1, 3), "operator", "secret")});
  const auto read = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureMessage,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 1,
       .symmetric_security_header =
           OpcUaBinarySymmetricSecurityHeader{.token_id = 1},
       .sequence_header = {.sequence_number = 4, .request_id = 12},
       .body = EncodeReadRequestBody(
           12, NumericNode(1, 3),
           {.node_id = NumericNode(99),
            .attribute_id = scada::AttributeId::Value})});

  peer->incoming.push_back(AsString(hello));
  peer->incoming.push_back(AsString(open));
  peer->incoming.push_back(AsString(create));
  peer->incoming.push_back(AsString(activate));
  peer->incoming.push_back(AsString(read));

  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(context.user_id(), expected_user_id_);
        EXPECT_EQ((*inputs)[0].node_id, NumericNode(99));
        callback(scada::StatusCode::Good,
                 {scada::DataValue{scada::Variant{12.5}, {}, now_, now_}});
      }));

  RunPeer(peer, dispatcher);

  ASSERT_GE(peer->writes.size(), 5u);
  const auto read_response_frame = DecodeSecureConversationMessage(
      std::vector<char>{peer->writes[4].begin(), peer->writes[4].end()});
  ASSERT_TRUE(read_response_frame.has_value());
  const auto value = DecodeSingleDoubleReadResponse(read_response_frame->body);
  ASSERT_TRUE(value.has_value());
  EXPECT_DOUBLE_EQ(*value, 12.5);
}

TEST_F(OpcUaBinaryServiceDispatcherTest, HandlesWriteAfterActivatedSession) {
  OpcUaBinaryServiceDispatcher dispatcher{
      {.runtime = runtime_,
       .session_manager = session_manager_,
       .connection = connection_}};

  const auto created = WaitAwaitable(
      executor_, dispatcher.HandlePayload(EncodeCreateSessionRequestBody(1, 45000)));
  ASSERT_TRUE(created.has_value());
  const auto session = DecodeCreateSessionResponse(*created);
  ASSERT_TRUE(session.has_value());

  const auto activated = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeUserNameActivateRequestBody(
          2, session->authentication_token, "operator", "secret")));
  ASSERT_TRUE(activated.has_value());

  EXPECT_CALL(attribute_service_, Write(_, _, _))
      .WillOnce(Invoke([this](const scada::ServiceContext& context,
                              const std::shared_ptr<const std::vector<scada::WriteValue>>&
                                  inputs,
                              const scada::WriteCallback& callback) {
        EXPECT_EQ(context.user_id(), expected_user_id_);
        ASSERT_EQ(inputs->size(), 1u);
        EXPECT_EQ((*inputs)[0].node_id, NumericNode(12));
        EXPECT_EQ((*inputs)[0].attribute_id, scada::AttributeId::Value);
        EXPECT_DOUBLE_EQ((*inputs)[0].value.get<scada::Double>(), 42.0);
        callback(scada::StatusCode::Good, {scada::StatusCode::Good});
      }));

  const auto written = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeWriteRequestBody(
          3, session->authentication_token,
          {.node_id = NumericNode(12),
           .attribute_id = scada::AttributeId::Value,
           .value = scada::Double{42.0}})));
  ASSERT_TRUE(written.has_value());
  const auto status = DecodeSingleWriteResponseStatus(*written);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(*status, 0u);
}

}  // namespace
}  // namespace opcua
