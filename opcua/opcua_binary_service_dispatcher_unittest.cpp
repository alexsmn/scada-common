#include "opcua/opcua_binary_service_dispatcher.h"
#include "opcua/opcua_binary_codec_utils.h"
#include "opcua/opcua_binary_service_codec.h"

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
constexpr std::uint32_t kBrowseRequestBinaryEncodingId = 525;
constexpr std::uint32_t kBrowseResponseBinaryEncodingId = 528;
constexpr std::uint32_t kCallRequestBinaryEncodingId = 710;
constexpr std::uint32_t kCallResponseBinaryEncodingId = 713;
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

void AppendRequestHeader(std::vector<char>& bytes,
                         const scada::NodeId& authentication_token,
                         std::uint32_t request_handle) {
  binary::BinaryEncoder encoder{bytes};
  encoder.Encode(authentication_token);
  encoder.Encode(std::int64_t{0});
  encoder.Encode(request_handle);
  encoder.Encode(std::uint32_t{0});
  encoder.Encode(std::string_view{""});
  encoder.Encode(std::uint32_t{0});
  encoder.Encode(scada::NodeId{});
  encoder.Encode(std::uint8_t{0x00});
}

std::vector<char> EncodeOpenRequestBody(std::uint32_t request_handle) {
  std::vector<char> payload;
  binary::BinaryEncoder payload_encoder{payload};
  payload_encoder.Encode(scada::NodeId{});
  payload_encoder.Encode(std::int64_t{0});
  payload_encoder.Encode(request_handle);
  payload_encoder.Encode(std::uint32_t{0});
  payload_encoder.Encode(std::string_view{""});
  payload_encoder.Encode(std::uint32_t{0});
  payload_encoder.Encode(scada::NodeId{});
  payload_encoder.Encode(std::uint8_t{0x00});
  payload_encoder.Encode(std::uint32_t{0});
  payload_encoder.Encode(
      static_cast<std::uint32_t>(OpcUaBinarySecurityTokenRequestType::Issue));
  payload_encoder.Encode(
      static_cast<std::uint32_t>(OpcUaBinaryMessageSecurityMode::None));
  payload_encoder.Encode(scada::ByteString{});
  payload_encoder.Encode(std::uint32_t{60000});

  std::vector<char> body;
  binary::BinaryEncoder body_encoder{body};
  body_encoder.Encode(binary::EncodedExtensionObject{
      .type_id = kOpenSecureChannelRequestBinaryEncodingId,
      .body = payload});
  return body;
}

std::vector<char> EncodeCreateSessionRequestBody(std::uint32_t request_handle,
                                                 double requested_timeout_ms) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = scada::NodeId{}, .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryCreateSessionRequest{
          .requested_timeout =
              base::TimeDelta::FromMillisecondsD(requested_timeout_ms)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeUserNameActivateRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    std::string_view user_name,
    std::string_view password) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryActivateSessionRequest{
          .authentication_token = authentication_token,
          .user_name = scada::ToLocalizedText(std::string{user_name}),
          .password = scada::ToLocalizedText(std::string{password}),
          .allow_anonymous = false}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeReadRequestBody(std::uint32_t request_handle,
                                        const scada::NodeId& authentication_token,
                                        const scada::ReadValueId& read_value_id) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryReadRequest{.inputs = {read_value_id}}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeWriteRequestBody(std::uint32_t request_handle,
                                         const scada::NodeId& authentication_token,
                                         const scada::WriteValue& write_value) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryWriteRequest{.inputs = {write_value}}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeBrowseRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    const scada::BrowseDescription& browse_description) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryBrowseRequest{.requested_max_references_per_node = 0,
                                   .inputs = {browse_description}}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeCallRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    const scada::NodeId& object_id,
    const scada::NodeId& method_id,
    const std::vector<scada::Variant>& arguments) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryCallRequest{.methods = {{.object_id = object_id,
                                              .method_id = method_id,
                                              .arguments = arguments}}}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
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
  const auto message = binary::ReadMessage(payload);
  if (!message.has_value() ||
      message->first != kCreateSessionResponseBinaryEncodingId) {
    return std::nullopt;
  }
  const auto& body = message->second;
  binary::BinaryDecoder decoder{body};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(session_id) ||
      !decoder.Decode(authentication_token)) {
    return std::nullopt;
  }
  return DecodedCreateSessionResponse{
      .session_id = session_id,
      .authentication_token = authentication_token,
  };
}

std::optional<double> DecodeSingleDoubleReadResponse(
    const std::vector<char>& payload) {
  const auto message = binary::ReadMessage(payload);
  if (!message.has_value() || message->first != kReadResponseBinaryEncodingId) {
    return std::nullopt;
  }
  const auto& body = message->second;

  binary::BinaryDecoder decoder{body};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte)) {
    return std::nullopt;
  }

  std::int32_t result_count = 0;
  if (!decoder.Decode(result_count) || result_count != 1) {
    return std::nullopt;
  }

  std::uint8_t data_value_mask = 0;
  if (!decoder.Decode(data_value_mask) || (data_value_mask & 0x01) == 0) {
    return std::nullopt;
  }

  std::uint8_t variant_mask = 0;
  double value = 0;
  if (!decoder.Decode(variant_mask) || variant_mask != 11 ||
      !decoder.Decode(value)) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::uint32_t> DecodeSingleWriteResponseStatus(
    const std::vector<char>& payload) {
  const auto message = binary::ReadMessage(payload);
  if (!message.has_value() || message->first != kWriteResponseBinaryEncodingId) {
    return std::nullopt;
  }
  const auto& body = message->second;

  binary::BinaryDecoder decoder{body};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte)) {
    return std::nullopt;
  }

  std::int32_t result_count = 0;
  std::uint32_t result_status = 0;
  if (!decoder.Decode(result_count) || result_count != 1 ||
      !decoder.Decode(result_status)) {
    return std::nullopt;
  }
  return result_status;
}

std::optional<scada::ReferenceDescription> DecodeSingleBrowseReference(
    const std::vector<char>& payload) {
  const auto message = binary::ReadMessage(payload);
  if (!message.has_value() || message->first != kBrowseResponseBinaryEncodingId) {
    return std::nullopt;
  }
  const auto& body = message->second;

  binary::BinaryDecoder decoder{body};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte)) {
    return std::nullopt;
  }

  std::int32_t result_count = 0;
  std::uint32_t result_status = 0;
  scada::ByteString ignored_continuation_point;
  std::int32_t reference_count = 0;
  scada::NodeId reference_type_id;
  bool forward = false;
  scada::ExpandedNodeId target_id;
  scada::QualifiedName ignored_browse_name;
  scada::LocalizedText ignored_display_name;
  std::uint32_t ignored_node_class = 0;
  scada::ExpandedNodeId ignored_type_definition;
  if (!decoder.Decode(result_count) || result_count != 1 ||
      !decoder.Decode(result_status) ||
      !decoder.Decode(ignored_continuation_point) ||
      !decoder.Decode(reference_count) || reference_count != 1 ||
      !decoder.Decode(reference_type_id) ||
      !decoder.Decode(forward) || !decoder.Decode(target_id) ||
      !decoder.Decode(ignored_browse_name) ||
      !decoder.Decode(ignored_display_name) ||
      !decoder.Decode(ignored_node_class) ||
      !decoder.Decode(ignored_type_definition)) {
    return std::nullopt;
  }

  return scada::ReferenceDescription{
      .reference_type_id = reference_type_id,
      .forward = forward,
      .node_id = target_id.node_id(),
  };
}

std::optional<std::uint32_t> DecodeSingleCallResponseStatus(
    const std::vector<char>& payload) {
  const auto message = binary::ReadMessage(payload);
  if (!message.has_value() || message->first != kCallResponseBinaryEncodingId) {
    return std::nullopt;
  }
  const auto& body = message->second;
  binary::BinaryDecoder decoder{body};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte)) {
    return std::nullopt;
  }

  std::int32_t result_count = 0;
  std::uint32_t method_status = 0;
  std::int32_t input_result_count = 0;
  std::int32_t input_diag_count = 0;
  std::int32_t output_argument_count = 0;
  if (!decoder.Decode(result_count) || result_count != 1 ||
      !decoder.Decode(method_status) ||
      !decoder.Decode(input_result_count) ||
      !decoder.Decode(input_diag_count) ||
      !decoder.Decode(output_argument_count)) {
    return std::nullopt;
  }
  if (input_result_count != 0 || input_diag_count != -1 ||
      output_argument_count != 0) {
    return std::nullopt;
  }
  return method_status;
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

TEST_F(OpcUaBinaryServiceDispatcherTest, HandlesBrowseAfterActivatedSession) {
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

  const scada::BrowseDescription browse_description{
      .node_id = NumericNode(12),
      .direction = scada::BrowseDirection::Forward,
      .reference_type_id = NumericNode(45),
      .include_subtypes = false,
  };
  EXPECT_CALL(view_service_, Browse(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::vector<scada::BrowseDescription>& inputs,
                           const scada::BrowseCallback& callback) {
        EXPECT_EQ(context.user_id(), expected_user_id_);
        ASSERT_EQ(inputs.size(), 1u);
        EXPECT_EQ(inputs[0], browse_description);
        callback(scada::StatusCode::Good,
                 {scada::BrowseResult{
                     .status_code = scada::StatusCode::Good,
                     .references = {{.reference_type_id = NumericNode(46),
                                     .forward = true,
                                     .node_id = NumericNode(99)}}}});
      }));

  const auto browsed = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeBrowseRequestBody(
          3, session->authentication_token, browse_description)));
  ASSERT_TRUE(browsed.has_value());
  const auto reference = DecodeSingleBrowseReference(*browsed);
  ASSERT_TRUE(reference.has_value());
  EXPECT_EQ(reference->reference_type_id, NumericNode(46));
  EXPECT_TRUE(reference->forward);
  EXPECT_EQ(reference->node_id, NumericNode(99));
}

TEST_F(OpcUaBinaryServiceDispatcherTest, HandlesCallAfterActivatedSession) {
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

  EXPECT_CALL(method_service_, Call(_, _, _, _, _))
      .WillOnce(Invoke([this](const scada::NodeId& node_id,
                              const scada::NodeId& method_id,
                              const std::vector<scada::Variant>& arguments,
                              const scada::NodeId& user_id,
                              const scada::StatusCallback& callback) {
        EXPECT_EQ(node_id, NumericNode(12));
        EXPECT_EQ(method_id, NumericNode(77));
        ASSERT_EQ(arguments.size(), 2u);
        EXPECT_DOUBLE_EQ(arguments[0].get<scada::Double>(), 42.0);
        EXPECT_EQ(arguments[1].get<scada::String>(), "go");
        EXPECT_EQ(user_id, expected_user_id_);
        callback(scada::StatusCode::Good);
      }));

  const auto called = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCallRequestBody(
          3, session->authentication_token, NumericNode(12), NumericNode(77),
          {scada::Double{42.0}, scada::String{"go"}})));
  ASSERT_TRUE(called.has_value());
  const auto status = DecodeSingleCallResponseStatus(*called);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(*status, 0u);
}

}  // namespace
}  // namespace opcua
