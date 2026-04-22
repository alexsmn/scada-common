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
#include "scada/monitoring_parameters.h"
#include "scada/node_management_service_mock.h"
#include "scada/test/test_monitored_item.h"
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
constexpr std::uint32_t kAddNodesRequestBinaryEncodingId = 488;
constexpr std::uint32_t kAddNodesResponseBinaryEncodingId = 491;
constexpr std::uint32_t kAddReferencesRequestBinaryEncodingId = 494;
constexpr std::uint32_t kAddReferencesResponseBinaryEncodingId = 497;
constexpr std::uint32_t kDeleteNodesRequestBinaryEncodingId = 500;
constexpr std::uint32_t kDeleteNodesResponseBinaryEncodingId = 503;
constexpr std::uint32_t kDeleteReferencesRequestBinaryEncodingId = 506;
constexpr std::uint32_t kDeleteReferencesResponseBinaryEncodingId = 509;
constexpr std::uint32_t kBrowseRequestBinaryEncodingId = 525;
constexpr std::uint32_t kBrowseResponseBinaryEncodingId = 528;
constexpr std::uint32_t kBrowseNextRequestBinaryEncodingId = 533;
constexpr std::uint32_t kBrowseNextResponseBinaryEncodingId = 536;
constexpr std::uint32_t kCreateSubscriptionRequestBinaryEncodingId = 787;
constexpr std::uint32_t kCreateSubscriptionResponseBinaryEncodingId = 790;
constexpr std::uint32_t kModifySubscriptionRequestBinaryEncodingId = 793;
constexpr std::uint32_t kModifySubscriptionResponseBinaryEncodingId = 796;
constexpr std::uint32_t kSetPublishingModeRequestBinaryEncodingId = 799;
constexpr std::uint32_t kSetPublishingModeResponseBinaryEncodingId = 802;
constexpr std::uint32_t kCreateMonitoredItemsRequestBinaryEncodingId = 751;
constexpr std::uint32_t kCreateMonitoredItemsResponseBinaryEncodingId = 754;
constexpr std::uint32_t kModifyMonitoredItemsRequestBinaryEncodingId = 763;
constexpr std::uint32_t kModifyMonitoredItemsResponseBinaryEncodingId = 766;
constexpr std::uint32_t kSetMonitoringModeRequestBinaryEncodingId = 769;
constexpr std::uint32_t kSetMonitoringModeResponseBinaryEncodingId = 772;
constexpr std::uint32_t kDeleteMonitoredItemsRequestBinaryEncodingId = 781;
constexpr std::uint32_t kDeleteMonitoredItemsResponseBinaryEncodingId = 784;
constexpr std::uint32_t kDeleteSubscriptionsRequestBinaryEncodingId = 847;
constexpr std::uint32_t kDeleteSubscriptionsResponseBinaryEncodingId = 850;
constexpr std::uint32_t kPublishRequestBinaryEncodingId = 826;
constexpr std::uint32_t kPublishResponseBinaryEncodingId = 829;
constexpr std::uint32_t kRepublishRequestBinaryEncodingId = 832;
constexpr std::uint32_t kRepublishResponseBinaryEncodingId = 835;
constexpr std::uint32_t kTransferSubscriptionsRequestBinaryEncodingId = 841;
constexpr std::uint32_t kTransferSubscriptionsResponseBinaryEncodingId = 844;
constexpr std::uint32_t kTranslateBrowsePathsRequestBinaryEncodingId = 554;
constexpr std::uint32_t kTranslateBrowsePathsResponseBinaryEncodingId = 557;
constexpr std::uint32_t kReadRawModifiedDetailsBinaryEncodingId = 649;
constexpr std::uint32_t kHistoryDataBinaryEncodingId = 658;
constexpr std::uint32_t kHistoryEventBinaryEncodingId = 661;
constexpr std::uint32_t kHistoryReadRequestBinaryEncodingId = 664;
constexpr std::uint32_t kHistoryReadResponseBinaryEncodingId = 667;
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
    const scada::BrowseDescription& browse_description,
    size_t requested_max_references_per_node = 0) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryBrowseRequest{
              .requested_max_references_per_node =
                  requested_max_references_per_node,
                                   .inputs = {browse_description}}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeBrowseNextRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    bool release_continuation_points,
    std::vector<scada::ByteString> continuation_points) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryBrowseNextRequest{
          .release_continuation_points = release_continuation_points,
          .continuation_points = std::move(continuation_points)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeHistoryReadRawRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    scada::HistoryReadRawDetails details) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryHistoryReadRawRequest{.details = std::move(details)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeHistoryReadEventsRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    scada::HistoryReadEventsDetails details) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryHistoryReadEventsRequest{.details = std::move(details)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeCreateSubscriptionRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    const OpcUaBinarySubscriptionParameters& parameters) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryCreateSubscriptionRequest{.parameters = parameters}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeModifySubscriptionRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    scada::UInt32 subscription_id,
    const OpcUaBinarySubscriptionParameters& parameters) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryModifySubscriptionRequest{
          .subscription_id = subscription_id, .parameters = parameters}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeSetPublishingModeRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    bool publishing_enabled,
    std::vector<scada::UInt32> subscription_ids) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinarySetPublishingModeRequest{
          .publishing_enabled = publishing_enabled,
          .subscription_ids = std::move(subscription_ids)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeDeleteSubscriptionsRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    std::vector<scada::UInt32> subscription_ids) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryDeleteSubscriptionsRequest{
          .subscription_ids = std::move(subscription_ids)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeCreateMonitoredItemsRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    scada::UInt32 subscription_id,
    std::vector<OpcUaBinaryMonitoredItemCreateRequest> items_to_create) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryCreateMonitoredItemsRequest{
          .subscription_id = subscription_id,
          .items_to_create = std::move(items_to_create)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeModifyMonitoredItemsRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    scada::UInt32 subscription_id,
    std::vector<OpcUaBinaryMonitoredItemModifyRequest> items_to_modify) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryModifyMonitoredItemsRequest{
          .subscription_id = subscription_id,
          .items_to_modify = std::move(items_to_modify)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodePublishRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    std::vector<OpcUaBinarySubscriptionAcknowledgement> acknowledgements = {}) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryPublishRequest{
          .subscription_acknowledgements = std::move(acknowledgements)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeDeleteMonitoredItemsRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    scada::UInt32 subscription_id,
    std::vector<scada::UInt32> monitored_item_ids) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryDeleteMonitoredItemsRequest{
          .subscription_id = subscription_id,
          .monitored_item_ids = std::move(monitored_item_ids)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeSetMonitoringModeRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    scada::UInt32 subscription_id,
    OpcUaBinaryMonitoringMode monitoring_mode,
    std::vector<scada::UInt32> monitored_item_ids) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinarySetMonitoringModeRequest{
          .subscription_id = subscription_id,
          .monitoring_mode = monitoring_mode,
          .monitored_item_ids = std::move(monitored_item_ids)}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeRepublishRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    scada::UInt32 subscription_id,
    scada::UInt32 retransmit_sequence_number) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryRepublishRequest{
          .subscription_id = subscription_id,
          .retransmit_sequence_number = retransmit_sequence_number}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeTransferSubscriptionsRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    std::vector<scada::UInt32> subscription_ids,
    bool send_initial_values) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryTransferSubscriptionsRequest{
          .subscription_ids = std::move(subscription_ids),
          .send_initial_values = send_initial_values}});
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

std::vector<char> EncodeTranslateBrowsePathsRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    const scada::BrowsePath& browse_path) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryTranslateBrowsePathsRequest{.inputs = {browse_path}}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeDeleteNodesRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    const scada::DeleteNodesItem& item) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryDeleteNodesRequest{.items = {item}}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeDeleteReferencesRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    const scada::DeleteReferencesItem& item) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryDeleteReferencesRequest{.items = {item}}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeAddReferencesRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    const scada::AddReferencesItem& item) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{
          OpcUaBinaryAddReferencesRequest{.items = {item}}});
  EXPECT_TRUE(encoded.has_value());
  return encoded.value_or(std::vector<char>{});
}

std::vector<char> EncodeAddNodesRequestBody(
    std::uint32_t request_handle,
    const scada::NodeId& authentication_token,
    const scada::AddNodesItem& item) {
  const auto encoded = EncodeOpcUaBinaryServiceRequest(
      {.authentication_token = authentication_token,
       .request_handle = request_handle},
      OpcUaBinaryRequestBody{OpcUaBinaryAddNodesRequest{.items = {item}}});
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
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kCreateSessionResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != kReadResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != kWriteResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != kBrowseResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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

std::optional<scada::BrowseResult> DecodeSingleBrowseResult(
    const std::vector<char>& payload,
    std::uint32_t expected_encoding_id) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != expected_encoding_id) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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
  scada::BrowseResult result;
  std::int32_t reference_count = 0;
  if (!decoder.Decode(result_count) || result_count != 1 ||
      !decoder.Decode(result_status) ||
      !decoder.Decode(result.continuation_point) ||
      !decoder.Decode(reference_count) || reference_count < 0) {
    return std::nullopt;
  }
  result.status_code =
      static_cast<scada::StatusCode>(static_cast<std::uint16_t>(result_status >> 16));
  result.references.resize(static_cast<std::size_t>(reference_count));
  for (auto& reference : result.references) {
    scada::ExpandedNodeId target_id;
    scada::QualifiedName ignored_browse_name;
    scada::LocalizedText ignored_display_name;
    std::uint32_t ignored_node_class = 0;
    scada::ExpandedNodeId ignored_type_definition;
    if (!decoder.Decode(reference.reference_type_id) ||
        !decoder.Decode(reference.forward) ||
        !decoder.Decode(target_id) ||
        !decoder.Decode(ignored_browse_name) ||
        !decoder.Decode(ignored_display_name) ||
        !decoder.Decode(ignored_node_class) ||
        !decoder.Decode(ignored_type_definition)) {
      return std::nullopt;
    }
    reference.node_id = target_id.node_id();
  }
  return result;
}

struct DecodedCreateSubscriptionResponse {
  scada::UInt32 subscription_id = 0;
  double revised_publishing_interval_ms = 0;
  scada::UInt32 revised_lifetime_count = 0;
  scada::UInt32 revised_max_keep_alive_count = 0;
};

std::optional<DecodedCreateSubscriptionResponse> DecodeCreateSubscriptionResponse(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kCreateSubscriptionResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  DecodedCreateSubscriptionResponse result;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) ||
      !decoder.Decode(result.subscription_id) ||
      !decoder.Decode(result.revised_publishing_interval_ms) ||
      !decoder.Decode(result.revised_lifetime_count) ||
      !decoder.Decode(result.revised_max_keep_alive_count)) {
    return std::nullopt;
  }
  return result;
}

struct DecodedModifySubscriptionResponse {
  std::uint32_t status = 0;
  double revised_publishing_interval_ms = 0;
  scada::UInt32 revised_lifetime_count = 0;
  scada::UInt32 revised_max_keep_alive_count = 0;
};

std::optional<DecodedModifySubscriptionResponse> DecodeModifySubscriptionResponse(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kModifySubscriptionResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  DecodedModifySubscriptionResponse result;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(result.status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) ||
      !decoder.Decode(result.revised_publishing_interval_ms) ||
      !decoder.Decode(result.revised_lifetime_count) ||
      !decoder.Decode(result.revised_max_keep_alive_count)) {
    return std::nullopt;
  }
  return result;
}

std::optional<std::uint32_t> DecodeSingleSetPublishingModeResponseStatus(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kSetPublishingModeResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int32_t result_count = 0;
  std::uint32_t result_status = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(result_count) ||
      result_count != 1 || !decoder.Decode(result_status)) {
    return std::nullopt;
  }
  return result_status;
}

std::optional<std::uint32_t> DecodeSingleDeleteSubscriptionsResponseStatus(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kDeleteSubscriptionsResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int32_t result_count = 0;
  std::uint32_t result_status = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(result_count) ||
      result_count != 1 || !decoder.Decode(result_status)) {
    return std::nullopt;
  }
  return result_status;
}

std::optional<scada::BrowseResult> DecodeSingleBrowseNextResult(
    const std::vector<char>& payload) {
  return DecodeSingleBrowseResult(payload, kBrowseNextResponseBinaryEncodingId);
}

std::optional<std::uint32_t> DecodeSingleCallResponseStatus(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != kCallResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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

std::optional<scada::BrowsePathTarget> DecodeSingleTranslateBrowsePathTarget(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kTranslateBrowsePathsResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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
  std::int32_t target_count = 0;
  scada::BrowsePathTarget target;
  if (!decoder.Decode(result_count) || result_count != 1 ||
      !decoder.Decode(result_status) ||
      !decoder.Decode(target_count) || target_count != 1 ||
      !decoder.Decode(target.target_id)) {
    return std::nullopt;
  }

  std::uint32_t remaining_path_index = 0;
  if (!decoder.Decode(remaining_path_index)) {
    return std::nullopt;
  }
  target.remaining_path_index = remaining_path_index;
  return target;
}

std::optional<std::uint32_t> DecodeSingleDeleteNodesResponseStatus(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kDeleteNodesResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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

std::optional<scada::AddNodesResult> DecodeSingleAddNodesResponseResult(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != kAddNodesResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  scada::AddNodesResult result;
  std::uint32_t result_status = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte)) {
    return std::nullopt;
  }

  std::int32_t result_count = 0;
  if (!decoder.Decode(result_count) || result_count != 1 ||
      !decoder.Decode(result_status) ||
      !decoder.Decode(result.added_node_id)) {
    return std::nullopt;
  }
  result.status_code = scada::Status::FromFullCode(result_status).code();
  return result;
}

std::optional<std::uint32_t> DecodeSingleDeleteReferencesResponseStatus(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kDeleteReferencesResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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

std::optional<std::uint32_t> DecodeSingleAddReferencesResponseStatus(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != kAddReferencesResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
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

std::optional<std::uint32_t> DecodeSingleResultStatus(
    const std::vector<char>& payload,
    std::uint32_t expected_type_id) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != expected_type_id) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t ignored_status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int32_t result_count = 0;
  std::uint32_t result_status = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(ignored_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(result_count) ||
      result_count != 1 || !decoder.Decode(result_status)) {
    return std::nullopt;
  }
  return result_status;
}

struct DecodedHistoryReadRawResponse {
  scada::Status status{scada::StatusCode::Bad};
  std::vector<scada::DataValue> values;
  scada::ByteString continuation_point;
};

std::optional<DecodedHistoryReadRawResponse> DecodeHistoryReadRawResponse(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kHistoryReadResponseBinaryEncodingId) {
    return std::nullopt;
  }

  binary::BinaryDecoder decoder{message->second};
  DecodedHistoryReadRawResponse response;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int32_t result_count = 0;
  std::uint32_t raw_status = 0;
  binary::DecodedExtensionObject history_data;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(raw_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(result_count) ||
      result_count != 1 || !decoder.Decode(raw_status) ||
      !decoder.Decode(response.continuation_point) ||
      !decoder.Decode(history_data) ||
      history_data.type_id != kHistoryDataBinaryEncodingId ||
      history_data.encoding != 0x01 || !decoder.Decode(ignored_array) ||
      !decoder.consumed()) {
    return std::nullopt;
  }

  response.status = scada::Status::FromFullCode(raw_status);
  binary::BinaryDecoder history_decoder{history_data.body};
  std::int32_t value_count = 0;
  if (!history_decoder.Decode(value_count) || value_count < 0) {
    return std::nullopt;
  }
  response.values.resize(static_cast<std::size_t>(value_count));
  for (auto& value : response.values) {
    std::uint8_t mask = 0;
    std::uint32_t ignored_status_code = 0;
    std::int64_t ignored_source_timestamp = 0;
    std::int64_t ignored_server_timestamp = 0;
    if (!history_decoder.Decode(mask)) {
      return std::nullopt;
    }
    if ((mask & 0x01) != 0 && !history_decoder.Decode(value.value)) {
      return std::nullopt;
    }
    if ((mask & 0x02) != 0 &&
        !history_decoder.Decode(ignored_status_code)) {
      return std::nullopt;
    }
    if ((mask & 0x04) != 0 &&
        !history_decoder.Decode(ignored_source_timestamp)) {
      return std::nullopt;
    }
    if ((mask & 0x08) != 0 &&
        !history_decoder.Decode(ignored_server_timestamp)) {
      return std::nullopt;
    }
  }
  if (!history_decoder.consumed()) {
    return std::nullopt;
  }
  return response;
}

struct DecodedHistoryReadEventsResponse {
  scada::Status status{scada::StatusCode::Bad};
  std::vector<std::vector<scada::Variant>> events;
};

std::optional<DecodedHistoryReadEventsResponse> DecodeHistoryReadEventsResponse(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kHistoryReadResponseBinaryEncodingId) {
    return std::nullopt;
  }

  binary::BinaryDecoder decoder{message->second};
  DecodedHistoryReadEventsResponse response;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int32_t result_count = 0;
  std::uint32_t raw_status = 0;
  scada::ByteString continuation_point;
  binary::DecodedExtensionObject history_event;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(raw_status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(result_count) ||
      result_count != 1 || !decoder.Decode(raw_status) ||
      !decoder.Decode(continuation_point) || !continuation_point.empty() ||
      !decoder.Decode(history_event) ||
      history_event.type_id != kHistoryEventBinaryEncodingId ||
      history_event.encoding != 0x01 || !decoder.Decode(ignored_array) ||
      !decoder.consumed()) {
    return std::nullopt;
  }

  response.status = scada::Status::FromFullCode(raw_status);
  binary::BinaryDecoder history_decoder{history_event.body};
  std::int32_t event_count = 0;
  if (!history_decoder.Decode(event_count) || event_count < 0) {
    return std::nullopt;
  }

  response.events.resize(static_cast<std::size_t>(event_count));
  for (auto& event_fields : response.events) {
    std::int32_t field_count = 0;
    if (!history_decoder.Decode(field_count) || field_count < 0) {
      return std::nullopt;
    }
    event_fields.resize(static_cast<std::size_t>(field_count));
    for (auto& field : event_fields) {
      if (!history_decoder.Decode(field)) {
        return std::nullopt;
      }
    }
  }
  if (!history_decoder.consumed()) {
    return std::nullopt;
  }
  return response;
}

struct DecodedCreateMonitoredItemsResponse {
  std::uint32_t status = 0;
  OpcUaBinaryMonitoredItemCreateResult result;
};

std::optional<DecodedCreateMonitoredItemsResponse>
DecodeCreateMonitoredItemsResponse(const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kCreateMonitoredItemsResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  DecodedCreateMonitoredItemsResponse response;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int32_t result_count = 0;
  std::uint32_t item_status = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(response.status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(result_count) ||
      result_count != 1 || !decoder.Decode(item_status) ||
      !decoder.Decode(response.result.monitored_item_id) ||
      !decoder.Decode(response.result.revised_sampling_interval_ms) ||
      !decoder.Decode(response.result.revised_queue_size)) {
    return std::nullopt;
  }
  binary::DecodedExtensionObject ignored_filter_result;
  if (!decoder.Decode(ignored_filter_result) ||
      !decoder.Decode(ignored_array)) {
    return std::nullopt;
  }
  response.result.status = scada::Status::FromFullCode(item_status);
  return response;
}

struct DecodedModifyMonitoredItemsResponse {
  std::uint32_t status = 0;
  OpcUaBinaryMonitoredItemModifyResult result;
};

std::optional<DecodedModifyMonitoredItemsResponse>
DecodeModifyMonitoredItemsResponse(const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kModifyMonitoredItemsResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  DecodedModifyMonitoredItemsResponse response;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int32_t result_count = 0;
  std::uint32_t item_status = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(response.status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(result_count) ||
      result_count != 1 || !decoder.Decode(item_status) ||
      !decoder.Decode(response.result.revised_sampling_interval_ms) ||
      !decoder.Decode(response.result.revised_queue_size)) {
    return std::nullopt;
  }
  binary::DecodedExtensionObject ignored_filter_result;
  if (!decoder.Decode(ignored_filter_result) ||
      !decoder.Decode(ignored_array)) {
    return std::nullopt;
  }
  response.result.status = scada::Status::FromFullCode(item_status);
  return response;
}

struct DecodedPublishResponse {
  std::uint32_t status = 0;
  scada::UInt32 subscription_id = 0;
  std::vector<scada::UInt32> available_sequence_numbers;
  bool more_notifications = false;
  scada::UInt32 sequence_number = 0;
  bool has_data_change = false;
  double data_change_value = 0;
  std::vector<std::uint32_t> acknowledgement_results;
};

std::optional<DecodedPublishResponse> DecodePublishResponse(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() || message->first != kPublishResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  DecodedPublishResponse response;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int32_t available_count = 0;
  std::int64_t ignored_publish_time = 0;
  std::int32_t notification_count = 0;
  binary::DecodedExtensionObject notification_data;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(response.status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(response.subscription_id) ||
      !decoder.Decode(available_count) || available_count < 0) {
    return std::nullopt;
  }
  response.available_sequence_numbers.resize(
      static_cast<std::size_t>(available_count));
  for (auto& sequence_number : response.available_sequence_numbers) {
    if (!decoder.Decode(sequence_number)) {
      return std::nullopt;
    }
  }
  if (!decoder.Decode(response.more_notifications) ||
      !decoder.Decode(response.sequence_number) ||
      !decoder.Decode(ignored_publish_time) || !decoder.Decode(notification_count) ||
      notification_count < 0) {
    return std::nullopt;
  }
  if (notification_count == 1) {
    std::int32_t item_count = 0;
    scada::UInt32 client_handle = 0;
    std::uint8_t value_mask = 0;
    scada::Variant value;
    std::uint32_t ignored_status_code = 0;
    std::int64_t ignored_source_timestamp = 0;
    std::int64_t ignored_server_timestamp = 0;
    if (!decoder.Decode(notification_data) ||
        notification_data.type_id != 811 || notification_data.encoding != 0x01) {
      return std::nullopt;
    }
    binary::BinaryDecoder notification_decoder{notification_data.body};
    if (!notification_decoder.Decode(item_count) || item_count != 1 ||
        !notification_decoder.Decode(client_handle) ||
        !notification_decoder.Decode(value_mask) || (value_mask & 0x01) == 0 ||
        !notification_decoder.Decode(value) ||
        value.type() != scada::Variant::DOUBLE) {
      return std::nullopt;
    }
    if ((value_mask & 0x02) != 0 &&
        !notification_decoder.Decode(ignored_status_code)) {
      return std::nullopt;
    }
    if ((value_mask & 0x04) != 0 &&
        !notification_decoder.Decode(ignored_source_timestamp)) {
      return std::nullopt;
    }
    if ((value_mask & 0x08) != 0 &&
        !notification_decoder.Decode(ignored_server_timestamp)) {
      return std::nullopt;
    }
    if (!notification_decoder.consumed()) {
      return std::nullopt;
    }
    response.has_data_change = true;
    response.data_change_value = value.get<scada::Double>();
  } else if (notification_count != 0) {
    return std::nullopt;
  }
  std::int32_t ack_count = 0;
  if (!decoder.Decode(ack_count) || ack_count < 0) {
    return std::nullopt;
  }
  response.acknowledgement_results.resize(static_cast<std::size_t>(ack_count));
  for (auto& result : response.acknowledgement_results) {
    if (!decoder.Decode(result)) {
      return std::nullopt;
    }
  }
  if (!decoder.Decode(ignored_array)) {
    return std::nullopt;
  }
  return response;
}

struct DecodedRepublishResponse {
  std::uint32_t status = 0;
  scada::UInt32 sequence_number = 0;
  double data_change_value = 0;
};

std::optional<DecodedRepublishResponse> DecodeRepublishResponse(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kRepublishResponseBinaryEncodingId) {
    return std::nullopt;
  }
  binary::BinaryDecoder decoder{message->second};
  DecodedRepublishResponse response;
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int64_t ignored_publish_time = 0;
  std::int32_t notification_count = 0;
  binary::DecodedExtensionObject notification_data;
  std::int32_t item_count = 0;
  scada::UInt32 client_handle = 0;
  std::uint8_t value_mask = 0;
  scada::Variant value;
  std::uint32_t ignored_status_code = 0;
  std::int64_t ignored_source_timestamp = 0;
  std::int64_t ignored_server_timestamp = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) ||
      !decoder.Decode(response.status) || !decoder.Decode(ignored_byte) ||
      !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) ||
      !decoder.Decode(response.sequence_number) ||
      !decoder.Decode(ignored_publish_time) ||
      !decoder.Decode(notification_count) || notification_count != 1 ||
      !decoder.Decode(notification_data) ||
      notification_data.type_id != 811 || notification_data.encoding != 0x01) {
    return std::nullopt;
  }
  binary::BinaryDecoder notification_decoder{notification_data.body};
  if (!notification_decoder.Decode(item_count) || item_count != 1 ||
      !notification_decoder.Decode(client_handle) ||
      !notification_decoder.Decode(value_mask) || (value_mask & 0x01) == 0 ||
      !notification_decoder.Decode(value) ||
      value.type() != scada::Variant::DOUBLE) {
    return std::nullopt;
  }
  if ((value_mask & 0x02) != 0 &&
      !notification_decoder.Decode(ignored_status_code)) {
    return std::nullopt;
  }
  if ((value_mask & 0x04) != 0 &&
      !notification_decoder.Decode(ignored_source_timestamp)) {
    return std::nullopt;
  }
  if ((value_mask & 0x08) != 0 &&
      !notification_decoder.Decode(ignored_server_timestamp)) {
    return std::nullopt;
  }
  if (!notification_decoder.consumed()) {
    return std::nullopt;
  }
  response.data_change_value = value.get<scada::Double>();
  return response;
}

std::optional<std::vector<std::uint32_t>> DecodeTransferSubscriptionsResponseResults(
    const std::vector<char>& payload) {
  binary::BinaryDecoder message_decoder{payload};
  const auto message = binary::ReadMessage(message_decoder);
  if (!message.has_value() ||
      message->first != kTransferSubscriptionsResponseBinaryEncodingId) {
    return std::nullopt;
  }

  binary::BinaryDecoder decoder{message->second};
  std::int64_t ignored_timestamp = 0;
  std::uint32_t ignored_request_handle = 0;
  std::uint32_t status = 0;
  std::uint8_t ignored_byte = 0;
  std::int32_t ignored_array = 0;
  scada::NodeId ignored_header_extension;
  std::int32_t result_count = 0;
  if (!decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(ignored_request_handle) || !decoder.Decode(status) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(ignored_array) ||
      !decoder.Decode(ignored_header_extension) ||
      !decoder.Decode(ignored_byte) || !decoder.Decode(result_count) ||
      result_count < 0) {
    return std::nullopt;
  }
  if (scada::Status::FromFullCode(status).code() != scada::StatusCode::Good) {
    return std::nullopt;
  }

  std::vector<std::uint32_t> results(static_cast<size_t>(result_count));
  for (auto& result : results) {
    if (!decoder.Decode(result)) {
      return std::nullopt;
    }
  }

  std::int32_t diagnostics = 0;
  if (!decoder.Decode(diagnostics) || diagnostics != -1 ||
      !decoder.consumed()) {
    return std::nullopt;
  }
  return results;
}

class TestMonitoredItemService : public scada::MonitoredItemService {
 public:
  std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& value_id,
      const scada::MonitoringParameters& parameters) override {
    created_value_ids.push_back(value_id);
    created_parameters.push_back(parameters);
    auto item = std::make_shared<scada::TestMonitoredItem>();
    items.push_back(item);
    return item;
  }

  std::vector<scada::ReadValueId> created_value_ids;
  std::vector<scada::MonitoringParameters> created_parameters;
  std::vector<std::shared_ptr<scada::TestMonitoredItem>> items;
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

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesBrowseNextAfterActivatedSession) {
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
                                     .node_id = NumericNode(99)},
                                    {.reference_type_id = NumericNode(47),
                                     .forward = true,
                                     .node_id = NumericNode(100)}}}});
      }));

  const auto browsed = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeBrowseRequestBody(
          3, session->authentication_token, browse_description, 1)));
  ASSERT_TRUE(browsed.has_value());

  const auto browse_page = DecodeSingleBrowseResult(*browsed, kBrowseResponseBinaryEncodingId);
  ASSERT_TRUE(browse_page.has_value());
  const auto paged_reference = DecodeSingleBrowseReference(*browsed);
  ASSERT_TRUE(paged_reference.has_value());
  EXPECT_EQ(paged_reference->reference_type_id, NumericNode(46));
  EXPECT_FALSE(browse_page->continuation_point.empty());

  const auto resumed = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(
          EncodeBrowseNextRequestBody(
              4, session->authentication_token, false,
              {browse_page->continuation_point})));
  ASSERT_TRUE(resumed.has_value());
  const auto result = DecodeSingleBrowseNextResult(*resumed);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->status_code, scada::StatusCode::Good);
  ASSERT_EQ(result->references.size(), 1u);
  EXPECT_EQ(result->references[0].reference_type_id, NumericNode(47));
  EXPECT_TRUE(result->references[0].forward);
  EXPECT_EQ(result->references[0].node_id, NumericNode(100));
  EXPECT_TRUE(result->continuation_point.empty());
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesTranslateBrowsePathsAfterActivatedSession) {
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

  const scada::BrowsePath browse_path{
      .node_id = NumericNode(12),
      .relative_path = {{
          .reference_type_id = NumericNode(33),
          .inverse = false,
          .include_subtypes = true,
          .target_name = {"Temperature", 2},
      }},
  };
  EXPECT_CALL(view_service_, TranslateBrowsePaths(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::BrowsePath>& inputs,
                           const scada::TranslateBrowsePathsCallback& callback) {
        ASSERT_EQ(inputs.size(), 1u);
        EXPECT_EQ(inputs[0], browse_path);
        callback(scada::StatusCode::Good,
                 {scada::BrowsePathResult{
                     .status_code = scada::StatusCode::Good,
                     .targets = {{.target_id = scada::ExpandedNodeId{
                                      NumericNode(77)},
                                  .remaining_path_index = 0}}}});
      }));

  const auto translated = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeTranslateBrowsePathsRequestBody(
          3, session->authentication_token, browse_path)));
  ASSERT_TRUE(translated.has_value());
  const auto target = DecodeSingleTranslateBrowsePathTarget(*translated);
  ASSERT_TRUE(target.has_value());
  EXPECT_EQ(target->target_id.node_id(), NumericNode(77));
  EXPECT_EQ(target->remaining_path_index, 0u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesHistoryReadRawAfterActivatedSession) {
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

  const auto from = now_ - base::TimeDelta::FromMinutes(15);
  const auto to = now_;
  EXPECT_CALL(history_service_, HistoryReadRaw(_, _))
      .WillOnce(Invoke([&](const scada::HistoryReadRawDetails& details,
                           const scada::HistoryReadRawCallback& callback) {
        EXPECT_EQ(details.node_id, NumericNode(120));
        EXPECT_EQ(details.from, from);
        EXPECT_EQ(details.to, to);
        EXPECT_EQ(details.max_count, 25u);
        EXPECT_EQ(details.continuation_point, (scada::ByteString{4, 5, 6}));
        callback(scada::HistoryReadRawResult{
            .status = scada::StatusCode::Good,
            .values = {scada::DataValue{
                scada::Variant{42.5},
                {},
                now_,
                now_,
            }},
            .continuation_point = {7, 8, 9},
        });
      }));

  const auto history_read = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeHistoryReadRawRequestBody(
          3, session->authentication_token,
          {.node_id = NumericNode(120),
           .from = from,
           .to = to,
           .max_count = 25,
           .continuation_point = {4, 5, 6}})));
  ASSERT_TRUE(history_read.has_value());
  const auto decoded = DecodeHistoryReadRawResponse(*history_read);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->status.code(), scada::StatusCode::Good);
  EXPECT_EQ(decoded->continuation_point, (scada::ByteString{7, 8, 9}));
  ASSERT_EQ(decoded->values.size(), 1u);
  ASSERT_EQ(decoded->values[0].value.type(), scada::Variant::DOUBLE);
  EXPECT_DOUBLE_EQ(decoded->values[0].value.get<scada::Double>(), 42.5);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesHistoryReadEventsAfterActivatedSession) {
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

  const auto from = now_ - base::TimeDelta::FromMinutes(30);
  const auto to = now_;
  const scada::EventFilter filter{
      .of_type = {NumericNode(301)},
      .child_of = {NumericNode(302)},
  };
  EXPECT_CALL(history_service_, HistoryReadEvents(_, _, _, _, _))
      .WillOnce(Invoke([&](const scada::NodeId& node_id,
                           base::Time actual_from,
                           base::Time actual_to,
                           const scada::EventFilter& actual_filter,
                           const scada::HistoryReadEventsCallback& callback) {
        EXPECT_EQ(node_id, NumericNode(300));
        EXPECT_EQ(actual_from, from);
        EXPECT_EQ(actual_to, to);
        EXPECT_EQ(actual_filter, filter);

        scada::Event event;
        event.event_id = 55;
        event.event_type_id = scada::id::SystemEventType;
        event.time = now_;
        event.receive_time = now_;
        event.node_id = NumericNode(303);
        event.message = scada::LocalizedText{u"alarm"};
        event.severity = 700;
        callback(scada::HistoryReadEventsResult{
            .status = scada::StatusCode::Good,
            .events = {std::move(event)},
        });
      }));

  const auto history_read = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeHistoryReadEventsRequestBody(
          3, session->authentication_token,
          {.node_id = NumericNode(300),
           .from = from,
           .to = to,
           .filter = filter})));
  ASSERT_TRUE(history_read.has_value());
  const auto decoded = DecodeHistoryReadEventsResponse(*history_read);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->status.code(), scada::StatusCode::Good);
  ASSERT_EQ(decoded->events.size(), 1u);
  ASSERT_EQ(decoded->events[0].size(), 7u);
  EXPECT_EQ(decoded->events[0][0].get<scada::UInt64>(), 55u);
  EXPECT_EQ(decoded->events[0][1].get<scada::NodeId>(),
            scada::NodeId{scada::id::SystemEventType});
  EXPECT_EQ(decoded->events[0][2].get<scada::NodeId>(), NumericNode(303));
  EXPECT_EQ(decoded->events[0][3].get<scada::String>(),
            NumericNode(303).ToString());
  EXPECT_EQ(decoded->events[0][4].get<scada::DateTime>(), now_);
  EXPECT_EQ(decoded->events[0][5].get<scada::LocalizedText>(),
            scada::LocalizedText{u"alarm"});
  EXPECT_EQ(decoded->events[0][6].get<scada::UInt32>(), 700u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesCreateSubscriptionAfterActivatedSession) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_NE(decoded->subscription_id, 0u);
  EXPECT_DOUBLE_EQ(decoded->revised_publishing_interval_ms, 100.0);
  EXPECT_EQ(decoded->revised_lifetime_count, 60u);
  EXPECT_EQ(decoded->revised_max_keep_alive_count, 3u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesModifySubscriptionAfterActivatedSession) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto modified = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeModifySubscriptionRequestBody(
          4, session->authentication_token, decoded_subscription->subscription_id,
          {.publishing_interval_ms = 250,
           .lifetime_count = 80,
           .max_keep_alive_count = 5,
           .publishing_enabled = true})));
  ASSERT_TRUE(modified.has_value());
  const auto decoded_modified = DecodeModifySubscriptionResponse(*modified);
  ASSERT_TRUE(decoded_modified.has_value());
  EXPECT_EQ(decoded_modified->status, 0u);
  EXPECT_DOUBLE_EQ(decoded_modified->revised_publishing_interval_ms, 250.0);
  EXPECT_EQ(decoded_modified->revised_lifetime_count, 80u);
  EXPECT_EQ(decoded_modified->revised_max_keep_alive_count, 5u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesSetPublishingModeAfterActivatedSession) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto set_mode = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeSetPublishingModeRequestBody(
          4, session->authentication_token, false,
          {decoded_subscription->subscription_id})));
  ASSERT_TRUE(set_mode.has_value());
  const auto status = DecodeSingleSetPublishingModeResponseStatus(*set_mode);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(*status, 0u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesDeleteSubscriptionsAfterActivatedSession) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded.has_value());

  const auto deleted = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeDeleteSubscriptionsRequestBody(
          4, session->authentication_token, {decoded->subscription_id})));
  ASSERT_TRUE(deleted.has_value());
  const auto status = DecodeSingleDeleteSubscriptionsResponseStatus(*deleted);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(*status, 0u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesCreateMonitoredItemsAfterActivatedSession) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto created_items = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateMonitoredItemsRequestBody(
          4, session->authentication_token,
          decoded_subscription->subscription_id,
          {{.item_to_monitor =
                {.node_id = NumericNode(11),
                 .attribute_id = scada::AttributeId::Value},
            .requested_parameters =
                {.client_handle = 44,
                 .sampling_interval_ms = 0,
                 .queue_size = 1,
                 .discard_oldest = true}}})));
  ASSERT_TRUE(created_items.has_value());
  const auto decoded = DecodeCreateMonitoredItemsResponse(*created_items);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->status, 0u);
  EXPECT_EQ(decoded->result.status.code(), scada::StatusCode::Good);
  EXPECT_NE(decoded->result.monitored_item_id, 0u);
  ASSERT_EQ(monitored_item_service_.created_value_ids.size(), 1u);
  EXPECT_EQ(monitored_item_service_.created_value_ids[0],
            (scada::ReadValueId{
                .node_id = NumericNode(11),
                .attribute_id = scada::AttributeId::Value}));
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesModifyMonitoredItemsAfterActivatedSession) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto created_items = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateMonitoredItemsRequestBody(
          4, session->authentication_token,
          decoded_subscription->subscription_id,
          {{.item_to_monitor =
                {.node_id = NumericNode(11),
                 .attribute_id = scada::AttributeId::Value},
            .requested_parameters =
                {.client_handle = 44,
                 .sampling_interval_ms = 0,
                 .queue_size = 1,
                 .discard_oldest = true}}})));
  ASSERT_TRUE(created_items.has_value());
  const auto decoded_created = DecodeCreateMonitoredItemsResponse(*created_items);
  ASSERT_TRUE(decoded_created.has_value());

  const auto modified = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeModifyMonitoredItemsRequestBody(
          5, session->authentication_token,
          decoded_subscription->subscription_id,
          {{.monitored_item_id = decoded_created->result.monitored_item_id,
            .requested_parameters =
                {.client_handle = 44,
                 .sampling_interval_ms = 250,
                 .queue_size = 3,
                 .discard_oldest = true}}})));
  ASSERT_TRUE(modified.has_value());
  const auto decoded_modified = DecodeModifyMonitoredItemsResponse(*modified);
  ASSERT_TRUE(decoded_modified.has_value());
  EXPECT_EQ(decoded_modified->status, 0u);
  EXPECT_EQ(decoded_modified->result.status.code(), scada::StatusCode::Good);
  EXPECT_DOUBLE_EQ(decoded_modified->result.revised_sampling_interval_ms, 250.0);
  EXPECT_EQ(decoded_modified->result.revised_queue_size, 3u);
  ASSERT_EQ(monitored_item_service_.created_parameters.size(), 2u);
  ASSERT_TRUE(monitored_item_service_.created_parameters[1].sampling_interval.has_value());
  EXPECT_EQ(*monitored_item_service_.created_parameters[1].sampling_interval,
            base::TimeDelta::FromMilliseconds(250));
  ASSERT_TRUE(monitored_item_service_.created_parameters[1].queue_size.has_value());
  EXPECT_EQ(*monitored_item_service_.created_parameters[1].queue_size, 3u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest, HandlesPublishAfterActivatedSession) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto created_items = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateMonitoredItemsRequestBody(
          4, session->authentication_token,
          decoded_subscription->subscription_id,
          {{.item_to_monitor =
                {.node_id = NumericNode(11),
                 .attribute_id = scada::AttributeId::Value},
            .requested_parameters =
                {.client_handle = 44,
                 .sampling_interval_ms = 0,
                 .queue_size = 1,
                 .discard_oldest = true}}})));
  ASSERT_TRUE(created_items.has_value());
  ASSERT_EQ(monitored_item_service_.items.size(), 1u);
  monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{12.5}, {}, now_, now_});
  now_ = now_ + base::TimeDelta::FromMilliseconds(100);

  const auto published = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(
          EncodePublishRequestBody(5, session->authentication_token)));
  ASSERT_TRUE(published.has_value());
  const auto decoded = DecodePublishResponse(*published);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->status, 0u);
  EXPECT_EQ(decoded->subscription_id, decoded_subscription->subscription_id);
  EXPECT_THAT(decoded->available_sequence_numbers,
              ElementsAre(decoded->sequence_number));
  EXPECT_FALSE(decoded->more_notifications);
  EXPECT_TRUE(decoded->has_data_change);
  EXPECT_DOUBLE_EQ(decoded->data_change_value, 12.5);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesPublishAcknowledgementsAndKeepAliveAfterDisablingPublishing) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 2,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto created_items = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateMonitoredItemsRequestBody(
          4, session->authentication_token,
          decoded_subscription->subscription_id,
          {{.item_to_monitor =
                {.node_id = NumericNode(11),
                 .attribute_id = scada::AttributeId::Value},
            .requested_parameters =
                {.client_handle = 44,
                 .sampling_interval_ms = 0,
                 .queue_size = 1,
                 .discard_oldest = true}}})));
  ASSERT_TRUE(created_items.has_value());
  ASSERT_EQ(monitored_item_service_.items.size(), 1u);
  monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{12.5}, {}, now_, now_});

  now_ = now_ + base::TimeDelta::FromMilliseconds(100);
  const auto published = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(
          EncodePublishRequestBody(5, session->authentication_token)));
  ASSERT_TRUE(published.has_value());
  const auto decoded_publish = DecodePublishResponse(*published);
  ASSERT_TRUE(decoded_publish.has_value());
  EXPECT_TRUE(decoded_publish->has_data_change);
  EXPECT_EQ(decoded_publish->acknowledgement_results.size(), 0u);

  const auto set_mode = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeSetPublishingModeRequestBody(
          6, session->authentication_token, false,
          {decoded_subscription->subscription_id})));
  ASSERT_TRUE(set_mode.has_value());
  const auto set_mode_status =
      DecodeSingleSetPublishingModeResponseStatus(*set_mode);
  ASSERT_TRUE(set_mode_status.has_value());
  EXPECT_EQ(*set_mode_status, 0u);

  now_ = now_ + base::TimeDelta::FromMilliseconds(300);
  const auto keep_alive = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodePublishRequestBody(
          7, session->authentication_token,
          {{.subscription_id = decoded_subscription->subscription_id,
            .sequence_number = decoded_publish->sequence_number}})));
  ASSERT_TRUE(keep_alive.has_value());
  const auto decoded_keep_alive = DecodePublishResponse(*keep_alive);
  ASSERT_TRUE(decoded_keep_alive.has_value());
  EXPECT_FALSE(decoded_keep_alive->has_data_change);
  EXPECT_EQ(decoded_keep_alive->subscription_id,
            decoded_subscription->subscription_id);
  EXPECT_EQ(decoded_keep_alive->sequence_number, 2u);
  EXPECT_THAT(decoded_keep_alive->acknowledgement_results, ElementsAre(0u));
}

TEST_F(OpcUaBinaryServiceDispatcherTest, HandlesRepublishAfterPublish) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto created_items = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateMonitoredItemsRequestBody(
          4, session->authentication_token,
          decoded_subscription->subscription_id,
          {{.item_to_monitor =
                {.node_id = NumericNode(11),
                 .attribute_id = scada::AttributeId::Value},
            .requested_parameters =
                {.client_handle = 44,
                 .sampling_interval_ms = 0,
                 .queue_size = 1,
                 .discard_oldest = true}}})));
  ASSERT_TRUE(created_items.has_value());
  ASSERT_EQ(monitored_item_service_.items.size(), 1u);
  monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{12.5}, {}, now_, now_});
  now_ = now_ + base::TimeDelta::FromMilliseconds(100);

  const auto published = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(
          EncodePublishRequestBody(5, session->authentication_token)));
  ASSERT_TRUE(published.has_value());
  const auto decoded_publish = DecodePublishResponse(*published);
  ASSERT_TRUE(decoded_publish.has_value());

  const auto republished = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeRepublishRequestBody(
          6, session->authentication_token,
          decoded_subscription->subscription_id,
          decoded_publish->sequence_number)));
  ASSERT_TRUE(republished.has_value());
  const auto decoded_republish = DecodeRepublishResponse(*republished);
  ASSERT_TRUE(decoded_republish.has_value());
  EXPECT_EQ(decoded_republish->status, 0u);
  EXPECT_EQ(decoded_republish->sequence_number,
            decoded_publish->sequence_number);
  EXPECT_DOUBLE_EQ(decoded_republish->data_change_value, 12.5);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesTransferSubscriptionsAcrossActivatedSessions) {
  OpcUaBinaryServiceDispatcher source_dispatcher{
      {.runtime = runtime_,
       .session_manager = session_manager_,
       .connection = connection_}};

  const auto created = WaitAwaitable(
      executor_,
      source_dispatcher.HandlePayload(EncodeCreateSessionRequestBody(1, 45000)));
  ASSERT_TRUE(created.has_value());
  const auto source_session = DecodeCreateSessionResponse(*created);
  ASSERT_TRUE(source_session.has_value());

  const auto activated = WaitAwaitable(
      executor_,
      source_dispatcher.HandlePayload(EncodeUserNameActivateRequestBody(
          2, source_session->authentication_token, "operator", "secret")));
  ASSERT_TRUE(activated.has_value());

  const auto subscription = WaitAwaitable(
      executor_,
      source_dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, source_session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto created_items = WaitAwaitable(
      executor_,
      source_dispatcher.HandlePayload(EncodeCreateMonitoredItemsRequestBody(
          4, source_session->authentication_token,
          decoded_subscription->subscription_id,
          {{.item_to_monitor =
                {.node_id = NumericNode(11),
                 .attribute_id = scada::AttributeId::Value},
            .requested_parameters =
                {.client_handle = 44,
                 .sampling_interval_ms = 0,
                 .queue_size = 1,
                 .discard_oldest = true}}})));
  ASSERT_TRUE(created_items.has_value());
  ASSERT_EQ(monitored_item_service_.items.size(), 1u);
  monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{77.0}, {}, now_, now_});

  OpcUaBinaryConnectionState target_connection;
  OpcUaBinaryServiceDispatcher target_dispatcher{
      {.runtime = runtime_,
       .session_manager = session_manager_,
       .connection = target_connection}};

  const auto target_created = WaitAwaitable(
      executor_,
      target_dispatcher.HandlePayload(EncodeCreateSessionRequestBody(5, 45000)));
  ASSERT_TRUE(target_created.has_value());
  const auto target_session = DecodeCreateSessionResponse(*target_created);
  ASSERT_TRUE(target_session.has_value());

  const auto target_activated = WaitAwaitable(
      executor_,
      target_dispatcher.HandlePayload(EncodeUserNameActivateRequestBody(
          6, target_session->authentication_token, "operator", "secret")));
  ASSERT_TRUE(target_activated.has_value());

  const auto transferred = WaitAwaitable(
      executor_,
      target_dispatcher.HandlePayload(EncodeTransferSubscriptionsRequestBody(
          7, target_session->authentication_token,
          {decoded_subscription->subscription_id}, true)));
  ASSERT_TRUE(transferred.has_value());
  const auto transfer_results =
      DecodeTransferSubscriptionsResponseResults(*transferred);
  ASSERT_TRUE(transfer_results.has_value());
  EXPECT_THAT(*transfer_results, ElementsAre(0u));

  now_ = now_ + base::TimeDelta::FromMilliseconds(100);
  const auto target_publish = WaitAwaitable(
      executor_,
      target_dispatcher.HandlePayload(
          EncodePublishRequestBody(8, target_session->authentication_token)));
  ASSERT_TRUE(target_publish.has_value());
  const auto target_decoded = DecodePublishResponse(*target_publish);
  ASSERT_TRUE(target_decoded.has_value());
  EXPECT_EQ(target_decoded->subscription_id,
            decoded_subscription->subscription_id);
  EXPECT_DOUBLE_EQ(target_decoded->data_change_value, 77.0);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesSetMonitoringModeAfterActivatedSession) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto created_items = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateMonitoredItemsRequestBody(
          4, session->authentication_token,
          decoded_subscription->subscription_id,
          {{.item_to_monitor =
                {.node_id = NumericNode(11),
                 .attribute_id = scada::AttributeId::Value},
            .requested_parameters =
                {.client_handle = 44,
                 .sampling_interval_ms = 0,
                 .queue_size = 1,
                 .discard_oldest = true}}})));
  ASSERT_TRUE(created_items.has_value());
  const auto decoded_items = DecodeCreateMonitoredItemsResponse(*created_items);
  ASSERT_TRUE(decoded_items.has_value());

  const auto set_mode = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeSetMonitoringModeRequestBody(
          5, session->authentication_token,
          decoded_subscription->subscription_id,
          OpcUaBinaryMonitoringMode::Sampling,
          {decoded_items->result.monitored_item_id})));
  ASSERT_TRUE(set_mode.has_value());
  const auto status = DecodeSingleResultStatus(
      *set_mode, kSetMonitoringModeResponseBinaryEncodingId);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(*status, 0u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesDeleteMonitoredItemsAfterActivatedSession) {
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

  const auto subscription = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateSubscriptionRequestBody(
          3, session->authentication_token,
          {.publishing_interval_ms = 100,
           .lifetime_count = 60,
           .max_keep_alive_count = 3,
           .publishing_enabled = true})));
  ASSERT_TRUE(subscription.has_value());
  const auto decoded_subscription = DecodeCreateSubscriptionResponse(*subscription);
  ASSERT_TRUE(decoded_subscription.has_value());

  const auto created_items = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeCreateMonitoredItemsRequestBody(
          4, session->authentication_token,
          decoded_subscription->subscription_id,
          {{.item_to_monitor =
                {.node_id = NumericNode(11),
                 .attribute_id = scada::AttributeId::Value},
            .requested_parameters =
                {.client_handle = 44,
                 .sampling_interval_ms = 0,
                 .queue_size = 1,
                 .discard_oldest = true}}})));
  ASSERT_TRUE(created_items.has_value());
  const auto decoded_items = DecodeCreateMonitoredItemsResponse(*created_items);
  ASSERT_TRUE(decoded_items.has_value());

  const auto deleted = WaitAwaitable(
      executor_,
      dispatcher.HandlePayload(EncodeDeleteMonitoredItemsRequestBody(
          5, session->authentication_token,
          decoded_subscription->subscription_id,
          {decoded_items->result.monitored_item_id})));
  ASSERT_TRUE(deleted.has_value());
  const auto status = DecodeSingleResultStatus(
      *deleted, kDeleteMonitoredItemsResponseBinaryEncodingId);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(*status, 0u);
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

TEST_F(OpcUaBinaryServiceDispatcherTest, HandlesDeleteNodesAfterActivatedSession) {
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

  const scada::DeleteNodesItem item{
      .node_id = NumericNode(12),
      .delete_target_references = true,
  };
  EXPECT_CALL(node_management_service_, DeleteNodes(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::DeleteNodesItem>& items,
                           const scada::DeleteNodesCallback& callback) {
        ASSERT_EQ(items.size(), 1u);
        EXPECT_EQ(items[0].node_id, item.node_id);
        EXPECT_EQ(items[0].delete_target_references,
                  item.delete_target_references);
        callback(scada::StatusCode::Good, {scada::StatusCode::Good});
      }));

  const auto deleted = WaitAwaitable(
      executor_, dispatcher.HandlePayload(EncodeDeleteNodesRequestBody(
                     3, session->authentication_token, item)));
  ASSERT_TRUE(deleted.has_value());
  const auto status = DecodeSingleDeleteNodesResponseStatus(*deleted);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(*status, 0u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest, HandlesAddNodesAfterActivatedSession) {
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

  const scada::AddNodesItem item{
      .requested_id = NumericNode(50),
      .parent_id = NumericNode(51),
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = NumericNode(52),
      .attributes = scada::NodeAttributes{}
                        .set_browse_name({"Flow", 2})
                        .set_display_name({u"Flow"})
                        .set_data_type(NumericNode(53)),
  };
  EXPECT_CALL(node_management_service_, AddNodes(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::AddNodesItem>& items,
                           const scada::AddNodesCallback& callback) {
        ASSERT_EQ(items.size(), 1u);
        EXPECT_EQ(items[0], item);
        callback(scada::StatusCode::Good,
                 {scada::AddNodesResult{
                     .status_code = scada::StatusCode::Good,
                     .added_node_id = NumericNode(54)}});        
      }));

  const auto added = WaitAwaitable(
      executor_, dispatcher.HandlePayload(EncodeAddNodesRequestBody(
                     3, session->authentication_token, item)));
  ASSERT_TRUE(added.has_value());
  const auto result = DecodeSingleAddNodesResponseResult(*added);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->status_code, scada::StatusCode::Good);
  EXPECT_EQ(result->added_node_id, NumericNode(54));
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesDeleteReferencesAfterActivatedSession) {
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

  const scada::DeleteReferencesItem item{
      .source_node_id = NumericNode(60),
      .reference_type_id = NumericNode(61),
      .forward = true,
      .target_node_id = scada::ExpandedNodeId{NumericNode(62)},
      .delete_bidirectional = true,
  };
  EXPECT_CALL(node_management_service_, DeleteReferences(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::DeleteReferencesItem>& items,
                           const scada::DeleteReferencesCallback& callback) {
        ASSERT_EQ(items.size(), 1u);
        EXPECT_EQ(items[0].source_node_id, item.source_node_id);
        EXPECT_EQ(items[0].reference_type_id, item.reference_type_id);
        EXPECT_EQ(items[0].forward, item.forward);
        EXPECT_EQ(items[0].target_node_id, item.target_node_id);
        EXPECT_EQ(items[0].delete_bidirectional, item.delete_bidirectional);
        callback(scada::StatusCode::Good, {scada::StatusCode::Good});
      }));

  const auto deleted = WaitAwaitable(
      executor_, dispatcher.HandlePayload(EncodeDeleteReferencesRequestBody(
                     3, session->authentication_token, item)));
  ASSERT_TRUE(deleted.has_value());
  const auto status = DecodeSingleDeleteReferencesResponseStatus(*deleted);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(*status, 0u);
}

TEST_F(OpcUaBinaryServiceDispatcherTest,
       HandlesAddReferencesAfterActivatedSession) {
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

  const scada::AddReferencesItem item{
      .source_node_id = NumericNode(70),
      .reference_type_id = NumericNode(71),
      .forward = true,
      .target_server_uri = "",
      .target_node_id = scada::ExpandedNodeId{NumericNode(72)},
      .target_node_class = scada::NodeClass::Object,
  };
  EXPECT_CALL(node_management_service_, AddReferences(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::AddReferencesItem>& items,
                           const scada::AddReferencesCallback& callback) {
        ASSERT_EQ(items.size(), 1u);
        EXPECT_EQ(items[0].source_node_id, item.source_node_id);
        EXPECT_EQ(items[0].reference_type_id, item.reference_type_id);
        EXPECT_EQ(items[0].forward, item.forward);
        EXPECT_EQ(items[0].target_server_uri, item.target_server_uri);
        EXPECT_EQ(items[0].target_node_id, item.target_node_id);
        EXPECT_EQ(items[0].target_node_class, item.target_node_class);
        callback(scada::StatusCode::Good, {scada::StatusCode::Good});
      }));

  const auto added = WaitAwaitable(
      executor_, dispatcher.HandlePayload(EncodeAddReferencesRequestBody(
                     3, session->authentication_token, item)));
  ASSERT_TRUE(added.has_value());
  const auto status = DecodeSingleAddReferencesResponseStatus(*added);
  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(*status, 0u);
}

}  // namespace
}  // namespace opcua
