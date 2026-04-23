#include "opcua/binary/opcua_binary_service_codec.h"

#include "opcua/binary/opcua_binary_codec_utils.h"
#include "opcua/opcua_endpoint_core.h"

#include "scada/localized_text.h"
#include "scada/standard_node_ids.h"

#include <opcuapp/structs.h>

namespace opcua {
namespace {

constexpr std::uint32_t kCreateSessionRequestBinaryEncodingId = 461;
constexpr std::uint32_t kCreateSessionResponseBinaryEncodingId = 464;
constexpr std::uint32_t kActivateSessionRequestBinaryEncodingId = 467;
constexpr std::uint32_t kActivateSessionResponseBinaryEncodingId = 470;
constexpr std::uint32_t kCloseSessionRequestBinaryEncodingId = 473;
constexpr std::uint32_t kCloseSessionResponseBinaryEncodingId = 476;
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
constexpr std::uint32_t kTranslateBrowsePathsRequestBinaryEncodingId = 554;
constexpr std::uint32_t kTranslateBrowsePathsResponseBinaryEncodingId = 557;
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
constexpr std::uint32_t kDeleteMonitoredItemsRequestBinaryEncodingId = 781;
constexpr std::uint32_t kDeleteMonitoredItemsResponseBinaryEncodingId = 784;
constexpr std::uint32_t kDeleteSubscriptionsRequestBinaryEncodingId = 847;
constexpr std::uint32_t kDeleteSubscriptionsResponseBinaryEncodingId = 850;
constexpr std::uint32_t kSetMonitoringModeRequestBinaryEncodingId = 769;
constexpr std::uint32_t kSetMonitoringModeResponseBinaryEncodingId = 772;
constexpr std::uint32_t kNotificationMessageBinaryEncodingId = 805;
constexpr std::uint32_t kDataChangeNotificationBinaryEncodingId = 811;
constexpr std::uint32_t kEventNotificationListBinaryEncodingId = 916;
constexpr std::uint32_t kEventFieldListBinaryEncodingId = 919;
constexpr std::uint32_t kStatusChangeNotificationBinaryEncodingId = 820;
constexpr std::uint32_t kPublishRequestBinaryEncodingId = 826;
constexpr std::uint32_t kPublishResponseBinaryEncodingId = 829;
constexpr std::uint32_t kRepublishRequestBinaryEncodingId = 832;
constexpr std::uint32_t kRepublishResponseBinaryEncodingId = 835;
constexpr std::uint32_t kTransferSubscriptionsRequestBinaryEncodingId = 841;
constexpr std::uint32_t kTransferSubscriptionsResponseBinaryEncodingId = 844;
constexpr std::uint32_t kCallRequestBinaryEncodingId = 710;
constexpr std::uint32_t kCallResponseBinaryEncodingId = 713;
constexpr std::uint32_t kReadEventDetailsBinaryEncodingId = 646;
constexpr std::uint32_t kReadRawModifiedDetailsBinaryEncodingId = 649;
constexpr std::uint32_t kHistoryDataBinaryEncodingId = 658;
constexpr std::uint32_t kHistoryEventBinaryEncodingId = 661;
constexpr std::uint32_t kHistoryReadRequestBinaryEncodingId = 664;
constexpr std::uint32_t kHistoryReadResponseBinaryEncodingId = 667;
constexpr std::uint32_t kDataChangeFilterBinaryEncodingId = 724;
constexpr std::uint32_t kEventFilterBinaryEncodingId = 727;
constexpr std::uint32_t kLiteralOperandBinaryEncodingId = 597;
constexpr std::uint32_t kSimpleAttributeOperandBinaryEncodingId = 603;
constexpr std::uint32_t kHistoryEventFieldListBinaryEncodingId = 922;
constexpr std::uint32_t kReadRequestBinaryEncodingId = 629;
constexpr std::uint32_t kReadResponseBinaryEncodingId = 632;
constexpr std::uint32_t kWriteRequestBinaryEncodingId = 671;
constexpr std::uint32_t kWriteResponseBinaryEncodingId = 674;
constexpr std::uint32_t kAnonymousIdentityTokenBinaryEncodingId = 321;
constexpr std::uint32_t kUserNameIdentityTokenBinaryEncodingId = 324;
constexpr std::uint32_t kObjectAttributesBinaryEncodingId = 354;
constexpr std::uint32_t kVariableAttributesBinaryEncodingId = 357;

enum class WireTimestampsToReturn : std::uint32_t {
  Source = 0,
  Server = 1,
  Both = 2,
  Neither = 3,
};

constexpr std::uint32_t EncodeSpecifiedAttribute(scada::AttributeId attribute_id) {
  return 1u << static_cast<unsigned>(attribute_id);
}

void AppendRequestHeader(binary::BinaryEncoder& encoder,
                         const OpcUaBinaryServiceRequestHeader& header) {
  encoder.Encode(header.authentication_token);
  encoder.Encode(std::int64_t{0});
  encoder.Encode(header.request_handle);
  encoder.Encode(std::uint32_t{0});
  encoder.Encode(std::string_view{""});
  encoder.Encode(std::uint32_t{0});
  encoder.Encode(scada::NodeId{});
  encoder.Encode(std::uint8_t{0x00});
}

void AppendResponseHeader(binary::BinaryEncoder& encoder,
                          std::uint32_t request_handle,
                          scada::Status status) {
  encoder.Encode(std::int64_t{0});
  encoder.Encode(request_handle);
  encoder.Encode(status.full_code());
  encoder.Encode(std::uint8_t{0});
  encoder.Encode(std::int32_t{0});
  encoder.Encode(scada::NodeId{});
  encoder.Encode(std::uint8_t{0x00});
}

std::uint32_t EncodeStatusCode(scada::StatusCode status_code) {
  return static_cast<std::uint32_t>(status_code) << 16;
}

void AppendDataValue(binary::BinaryEncoder& encoder,
                     const scada::DataValue& value) {
  std::uint8_t mask = 0;
  if (!value.value.is_null()) {
    mask |= 0x01;
  }
  if (!scada::IsGood(value.status_code)) {
    mask |= 0x02;
  }
  if (!value.source_timestamp.is_null()) {
    mask |= 0x04;
  }
  if (!value.server_timestamp.is_null()) {
    mask |= 0x08;
  }

  encoder.Encode(mask);
  if ((mask & 0x01) != 0) {
    encoder.Encode(value.value);
  }
  if ((mask & 0x02) != 0) {
    encoder.Encode(EncodeStatusCode(value.status_code));
  }
  if ((mask & 0x04) != 0) {
    encoder.Encode(value.source_timestamp);
  }
  if ((mask & 0x08) != 0) {
    encoder.Encode(value.server_timestamp);
  }
}

void AppendReferenceDescription(binary::BinaryEncoder& encoder,
                                const scada::ReferenceDescription& reference) {
  encoder.Encode(reference.reference_type_id);
  encoder.Encode(reference.forward);
  encoder.Encode(scada::ExpandedNodeId{reference.node_id});
  encoder.Encode(scada::QualifiedName{});
  encoder.Encode(scada::LocalizedText{});
  encoder.Encode(std::uint32_t{0});
  encoder.Encode(scada::ExpandedNodeId{});
}

void AppendRelativePathElement(binary::BinaryEncoder& encoder,
                               const scada::RelativePathElement& element) {
  encoder.Encode(element.reference_type_id);
  encoder.Encode(element.inverse);
  encoder.Encode(element.include_subtypes);
  encoder.Encode(element.target_name);
}

void AppendBrowsePath(binary::BinaryEncoder& encoder,
                      const scada::BrowsePath& path) {
  encoder.Encode(path.node_id);
  encoder.Encode(static_cast<std::int32_t>(path.relative_path.size()));
  for (const auto& element : path.relative_path) {
    AppendRelativePathElement(encoder, element);
  }
}

void AppendBrowsePathTarget(binary::BinaryEncoder& encoder,
                            const scada::BrowsePathTarget& target) {
  encoder.Encode(target.target_id);
  encoder.Encode(static_cast<std::uint32_t>(target.remaining_path_index));
}

void AppendBrowsePathResult(binary::BinaryEncoder& encoder,
                            const scada::BrowsePathResult& result) {
  encoder.Encode(EncodeStatusCode(result.status_code));
  encoder.Encode(static_cast<std::int32_t>(result.targets.size()));
  for (const auto& target : result.targets) {
    AppendBrowsePathTarget(encoder, target);
  }
}

void AppendHistoryData(binary::BinaryEncoder& encoder,
                       const scada::HistoryReadRawResult& result) {
  std::vector<char> body;
  binary::BinaryEncoder body_encoder{body};
  body_encoder.Encode(static_cast<std::int32_t>(result.values.size()));
  for (const auto& value : result.values) {
    AppendDataValue(body_encoder, value);
  }
  encoder.Encode(binary::EncodedExtensionObject{
      .type_id = kHistoryDataBinaryEncodingId,
      .body = std::move(body),
  });
}

void AppendLiteralOperand(binary::BinaryEncoder& encoder,
                          const scada::Variant& value) {
  std::vector<char> body;
  binary::BinaryEncoder body_encoder{body};
  body_encoder.Encode(value);
  encoder.Encode(binary::EncodedExtensionObject{
      .type_id = kLiteralOperandBinaryEncodingId,
      .body = std::move(body),
  });
}

void AppendSimpleAttributeOperand(
    binary::BinaryEncoder& encoder,
    std::span<const std::string> browse_path) {
  std::vector<char> body;
  binary::BinaryEncoder body_encoder{body};
  body_encoder.Encode(scada::NodeId{scada::id::BaseEventType});
  body_encoder.Encode(static_cast<std::int32_t>(browse_path.size()));
  for (const auto& segment : browse_path) {
    body_encoder.Encode(scada::QualifiedName{segment, 0});
  }
  body_encoder.Encode(static_cast<std::uint32_t>(scada::AttributeId::Value));
  body_encoder.Encode(std::string_view{""});
  encoder.Encode(binary::EncodedExtensionObject{
      .type_id = kSimpleAttributeOperandBinaryEncodingId,
      .body = std::move(body),
  });
}

void AppendEventFilter(binary::BinaryEncoder& encoder,
                       std::span<const std::vector<std::string>> field_paths,
                       const scada::EventFilter& filter) {
  std::vector<char> body;
  binary::BinaryEncoder body_encoder{body};
  body_encoder.Encode(static_cast<std::int32_t>(field_paths.size()));
  for (const auto& field_path : field_paths) {
    AppendSimpleAttributeOperand(body_encoder, field_path);
  }

  body_encoder.Encode(static_cast<std::int32_t>(filter.of_type.size() +
                                                filter.child_of.size()));
  for (const auto& of_type : filter.of_type) {
    body_encoder.Encode(
        static_cast<std::uint32_t>(OpcUa_FilterOperator_OfType));
    body_encoder.Encode(std::int32_t{1});
    AppendLiteralOperand(body_encoder, scada::Variant{of_type});
  }
  for (const auto& child_of : filter.child_of) {
    body_encoder.Encode(
        static_cast<std::uint32_t>(OpcUa_FilterOperator_RelatedTo));
    body_encoder.Encode(std::int32_t{1});
    AppendLiteralOperand(body_encoder, scada::Variant{child_of});
  }

  encoder.Encode(binary::EncodedExtensionObject{
      .type_id = kEventFilterBinaryEncodingId,
      .body = std::move(body),
  });
}

void AppendHistoryEvent(binary::BinaryEncoder& encoder,
                        const HistoryReadEventsResponse& response,
                        std::span<const std::vector<std::string>> field_paths) {
  const auto paths = scada::opcua_endpoint::NormalizeEventFieldPaths(
      std::vector<std::vector<std::string>>(field_paths.begin(),
                                            field_paths.end()));
  std::vector<char> body;
  binary::BinaryEncoder body_encoder{body};
  body_encoder.Encode(static_cast<std::int32_t>(response.result.events.size()));
  for (const auto& event : response.result.events) {
    const auto event_fields =
        scada::opcua_endpoint::ProjectEventFields(paths, std::any{event});
    body_encoder.Encode(static_cast<std::int32_t>(event_fields.size()));
    for (const auto& field : event_fields) {
      body_encoder.Encode(field);
    }
  }
  encoder.Encode(binary::EncodedExtensionObject{
      .type_id = kHistoryEventBinaryEncodingId,
      .body = std::move(body),
  });
}

void AppendDataChangeFilter(binary::BinaryEncoder& encoder,
                            const OpcUaDataChangeFilter& filter) {
  std::vector<char> body;
  binary::BinaryEncoder body_encoder{body};
  body_encoder.Encode(static_cast<std::uint32_t>(filter.trigger));
  body_encoder.Encode(static_cast<std::uint32_t>(filter.deadband_type));
  body_encoder.Encode(filter.deadband_value);
  encoder.Encode(binary::EncodedExtensionObject{
      .type_id = kDataChangeFilterBinaryEncodingId,
      .body = std::move(body),
  });
}

void AppendNullExtensionObject(binary::BinaryEncoder& encoder) {
  encoder.Encode(scada::NodeId{});
  encoder.Encode(std::uint8_t{0x00});
}

void AppendMonitoringFilter(
    binary::BinaryEncoder& encoder,
    const std::optional<OpcUaMonitoringFilter>& filter) {
  if (!filter.has_value()) {
    AppendNullExtensionObject(encoder);
    return;
  }

  std::visit(
      [&encoder](const auto& typed_filter) {
        using T = std::decay_t<decltype(typed_filter)>;
        if constexpr (std::is_same_v<T, OpcUaDataChangeFilter>) {
          AppendDataChangeFilter(encoder, typed_filter);
        } else {
          AppendEventFilter(
              encoder,
              scada::opcua_endpoint::ParseEventFilterFieldPaths(typed_filter),
              scada::EventFilter{});
        }
      },
      *filter);
}

void AppendMonitoredItemCreateRequest(
    binary::BinaryEncoder& encoder,
    const OpcUaMonitoredItemCreateRequest& request) {
  encoder.Encode(request.item_to_monitor.node_id);
  encoder.Encode(static_cast<std::uint32_t>(request.item_to_monitor.attribute_id));
  encoder.Encode(request.index_range.value_or(scada::String{}));
  encoder.Encode(scada::QualifiedName{});
  encoder.Encode(static_cast<std::uint32_t>(request.monitoring_mode));
  encoder.Encode(request.requested_parameters.client_handle);
  encoder.Encode(request.requested_parameters.sampling_interval_ms);
  AppendMonitoringFilter(encoder, request.requested_parameters.filter);
  encoder.Encode(request.requested_parameters.queue_size);
  encoder.Encode(request.requested_parameters.discard_oldest);
}

void AppendMonitoredItemCreateResult(
    binary::BinaryEncoder& encoder,
    const OpcUaMonitoredItemCreateResult& result) {
  encoder.Encode(result.status.full_code());
  encoder.Encode(result.monitored_item_id);
  encoder.Encode(result.revised_sampling_interval_ms);
  encoder.Encode(result.revised_queue_size);
  AppendNullExtensionObject(encoder);
}

void AppendMonitoredItemModifyResult(
    binary::BinaryEncoder& encoder,
    const OpcUaMonitoredItemModifyResult& result) {
  encoder.Encode(result.status.full_code());
  encoder.Encode(result.revised_sampling_interval_ms);
  encoder.Encode(result.revised_queue_size);
  AppendNullExtensionObject(encoder);
}

void AppendNotificationData(binary::BinaryEncoder& encoder,
                            const OpcUaNotificationData& data);

void AppendNotificationMessage(binary::BinaryEncoder& encoder,
                               const OpcUaNotificationMessage& message) {
  encoder.Encode(message.sequence_number);
  encoder.Encode(message.publish_time);
  encoder.Encode(static_cast<std::int32_t>(message.notification_data.size()));
  for (const auto& data : message.notification_data) {
    AppendNotificationData(encoder, data);
  }
}

void AppendNotificationData(binary::BinaryEncoder& encoder,
                            const OpcUaNotificationData& data) {
  std::visit(
      [&](const auto& typed) {
        using T = std::decay_t<decltype(typed)>;
        std::vector<char> body;
        binary::BinaryEncoder body_encoder{body};
        std::uint32_t type_id = 0;
        if constexpr (std::is_same_v<T, OpcUaDataChangeNotification>) {
          type_id = kDataChangeNotificationBinaryEncodingId;
          body_encoder.Encode(
              static_cast<std::int32_t>(typed.monitored_items.size()));
          for (const auto& item : typed.monitored_items) {
            body_encoder.Encode(item.client_handle);
            AppendDataValue(body_encoder, item.value);
          }
        } else if constexpr (std::is_same_v<T, OpcUaEventNotificationList>) {
          type_id = kEventNotificationListBinaryEncodingId;
          body_encoder.Encode(static_cast<std::int32_t>(typed.events.size()));
          for (const auto& event : typed.events) {
            std::vector<char> event_body;
            binary::BinaryEncoder event_encoder{event_body};
            event_encoder.Encode(event.client_handle);
            event_encoder.Encode(static_cast<std::int32_t>(event.event_fields.size()));
            for (const auto& field : event.event_fields) {
              event_encoder.Encode(field);
            }
            body_encoder.Encode(binary::EncodedExtensionObject{
                .type_id = kEventFieldListBinaryEncodingId, .body = event_body});
          }
        } else if constexpr (std::is_same_v<T, OpcUaStatusChangeNotification>) {
          type_id = kStatusChangeNotificationBinaryEncodingId;
          body_encoder.Encode(EncodeStatusCode(typed.status));
        }
        encoder.Encode(binary::EncodedExtensionObject{.type_id = type_id, .body = body});
      },
      data);
}

std::optional<binary::EncodedExtensionObject> EncodeNodeAttributesExtension(
    scada::NodeClass node_class,
    const scada::NodeAttributes& attributes) {
  std::vector<char> body;
  binary::BinaryEncoder encoder{body};
  std::uint32_t specified_attributes = 0;
  if (!attributes.display_name.empty()) {
    specified_attributes |=
        EncodeSpecifiedAttribute(scada::AttributeId::DisplayName);
  }
  if (!attributes.value.has_value() && attributes.data_type.is_null() &&
      node_class == scada::NodeClass::Variable) {
    specified_attributes |= 0;
  }

  switch (node_class) {
    case scada::NodeClass::Object: {
      encoder.Encode(specified_attributes);
      encoder.Encode(attributes.display_name);
      encoder.Encode(scada::LocalizedText{});
      encoder.Encode(std::uint32_t{0});
      encoder.Encode(std::uint32_t{0});
      encoder.Encode(std::uint8_t{0});
      return binary::EncodedExtensionObject{
          .type_id = kObjectAttributesBinaryEncodingId, .body = body};
    }

    case scada::NodeClass::Variable: {
      if (attributes.value.has_value()) {
        specified_attributes |=
            EncodeSpecifiedAttribute(scada::AttributeId::Value);
      }
      if (!attributes.data_type.is_null()) {
        specified_attributes |=
            EncodeSpecifiedAttribute(scada::AttributeId::DataType);
      }
      encoder.Encode(specified_attributes);
      encoder.Encode(attributes.display_name);
      encoder.Encode(scada::LocalizedText{});
      encoder.Encode(std::uint32_t{0});
      encoder.Encode(std::uint32_t{0});
      encoder.Encode(attributes.value.value_or(scada::Variant{}));
      encoder.Encode(attributes.data_type);
      encoder.Encode(std::int32_t{-1});
      encoder.Encode(std::int32_t{-1});
      encoder.Encode(std::uint8_t{0});
      encoder.Encode(std::uint8_t{0});
      encoder.Encode(0.0);
      encoder.Encode(false);
      return binary::EncodedExtensionObject{
          .type_id = kVariableAttributesBinaryEncodingId, .body = body};
    }

    default:
      return std::nullopt;
  }
}

bool DecodeNodeAttributesObject(binary::BinaryDecoder& decoder,
                                scada::NodeAttributes& attributes) {
  std::uint32_t specified_attributes = 0;
  scada::LocalizedText display_name;
  scada::LocalizedText ignored_description;
  std::uint32_t ignored_write_mask = 0;
  std::uint32_t ignored_user_write_mask = 0;
  std::uint8_t ignored_event_notifier = 0;
  if (!decoder.Decode(specified_attributes) || !decoder.Decode(display_name) ||
      !decoder.Decode(ignored_description) ||
      !decoder.Decode(ignored_write_mask) ||
      !decoder.Decode(ignored_user_write_mask) ||
      !decoder.Decode(ignored_event_notifier) || !decoder.consumed()) {
    return false;
  }

  if ((specified_attributes &
       EncodeSpecifiedAttribute(scada::AttributeId::DisplayName)) != 0) {
    attributes.set_display_name(std::move(display_name));
  }
  return true;
}

bool DecodeNodeAttributesVariable(binary::BinaryDecoder& decoder,
                                  scada::NodeAttributes& attributes) {
  std::uint32_t specified_attributes = 0;
  scada::LocalizedText display_name;
  scada::LocalizedText ignored_description;
  std::uint32_t ignored_write_mask = 0;
  std::uint32_t ignored_user_write_mask = 0;
  scada::Variant value;
  scada::NodeId data_type;
  std::int32_t ignored_value_rank = 0;
  std::int32_t ignored_array_dimensions = 0;
  std::uint8_t ignored_access_level = 0;
  std::uint8_t ignored_user_access_level = 0;
  double ignored_minimum_sampling_interval = 0;
  bool ignored_historizing = false;
  if (!decoder.Decode(specified_attributes) || !decoder.Decode(display_name) ||
      !decoder.Decode(ignored_description) ||
      !decoder.Decode(ignored_write_mask) ||
      !decoder.Decode(ignored_user_write_mask) || !decoder.Decode(value) ||
      !decoder.Decode(data_type) || !decoder.Decode(ignored_value_rank) ||
      !decoder.Decode(ignored_array_dimensions) ||
      !decoder.Decode(ignored_access_level) ||
      !decoder.Decode(ignored_user_access_level) ||
      !decoder.Decode(ignored_minimum_sampling_interval) ||
      !decoder.Decode(ignored_historizing) || !decoder.consumed()) {
    return false;
  }

  if ((specified_attributes &
       EncodeSpecifiedAttribute(scada::AttributeId::DisplayName)) != 0) {
    attributes.set_display_name(std::move(display_name));
  }
  if ((specified_attributes &
       EncodeSpecifiedAttribute(scada::AttributeId::Value)) != 0) {
    attributes.set_value(std::move(value));
  }
  if ((specified_attributes &
       EncodeSpecifiedAttribute(scada::AttributeId::DataType)) != 0) {
    attributes.set_data_type(std::move(data_type));
  }
  return true;
}

bool DecodeNodeAttributesExtension(binary::BinaryDecoder& decoder,
                                   scada::NodeClass node_class,
                                   scada::NodeAttributes& attributes) {
  binary::DecodedExtensionObject extension;
  if (!decoder.Decode(extension) || extension.encoding != 0x01) {
    return false;
  }

  binary::BinaryDecoder body_decoder{extension.body};
  switch (node_class) {
    case scada::NodeClass::Object:
      return extension.type_id == kObjectAttributesBinaryEncodingId &&
             DecodeNodeAttributesObject(body_decoder, attributes);
    case scada::NodeClass::Variable:
      return extension.type_id == kVariableAttributesBinaryEncodingId &&
             DecodeNodeAttributesVariable(body_decoder, attributes);
    default:
      return false;
  }
}

bool SkipStringArray(binary::BinaryDecoder& decoder) {
  std::int32_t count = 0;
  if (!decoder.Decode(count)) {
    return false;
  }
  if (count < 0) {
    return true;
  }
  for (std::int32_t i = 0; i < count; ++i) {
    std::string ignored;
    if (!decoder.Decode(ignored)) {
      return false;
    }
  }
  return true;
}

bool SkipLocalizedText(binary::BinaryDecoder& decoder) {
  std::uint8_t mask = 0;
  if (!decoder.Decode(mask)) {
    return false;
  }
  if ((mask & 0x01) != 0) {
    std::string locale;
    if (!decoder.Decode(locale)) {
      return false;
    }
  }
  if ((mask & 0x02) != 0) {
    std::string text;
    if (!decoder.Decode(text)) {
      return false;
    }
  }
  return true;
}

bool SkipApplicationDescriptionFields(binary::BinaryDecoder& decoder) {
  std::string ignored;
  std::int32_t application_type = 0;
  return decoder.Decode(ignored) && decoder.Decode(ignored) &&
         SkipLocalizedText(decoder) && decoder.Decode(application_type) &&
         decoder.Decode(ignored) && decoder.Decode(ignored) &&
         SkipStringArray(decoder);
}

bool SkipSignatureData(binary::BinaryDecoder& decoder) {
  std::string algorithm;
  scada::ByteString signature;
  return decoder.Decode(algorithm) && decoder.Decode(signature);
}

bool SkipSignedSoftwareCertificates(binary::BinaryDecoder& decoder) {
  std::int32_t count = 0;
  if (!decoder.Decode(count)) {
    return false;
  }
  if (count < 0) {
    return true;
  }
  for (std::int32_t i = 0; i < count; ++i) {
    scada::ByteString certificate_data;
    scada::ByteString signature;
    if (!decoder.Decode(certificate_data) ||
        !decoder.Decode(signature)) {
      return false;
    }
  }
  return true;
}

bool ReadRequestHeader(binary::BinaryDecoder& decoder,
                       OpcUaBinaryServiceRequestHeader& header) {
  std::int64_t ignored_timestamp = 0;
  if (!decoder.Decode(header.authentication_token) ||
      !decoder.Decode(ignored_timestamp) ||
      !decoder.Decode(header.request_handle)) {
    return false;
  }

  std::uint32_t ignored_u32 = 0;
  std::string ignored_string;
  if (!decoder.Decode(ignored_u32) || !decoder.Decode(ignored_string) ||
      !decoder.Decode(ignored_u32)) {
    return false;
  }

  binary::DecodedExtensionObject additional;
  return decoder.Decode(additional);
}

std::optional<scada::Variant> DecodeDataValue(binary::BinaryDecoder& decoder) {
  std::uint8_t mask = 0;
  if (!decoder.Decode(mask) || (mask & 0xf0) != 0) {
    return std::nullopt;
  }

  scada::Variant value;
  if ((mask & 0x01) != 0) {
    if (!decoder.Decode(value)) {
      return std::nullopt;
    }
  }

  if ((mask & 0x02) != 0) {
    std::uint32_t ignored_status = 0;
    if (!decoder.Decode(ignored_status)) {
      return std::nullopt;
    }
  }
  if ((mask & 0x04) != 0) {
    std::int64_t ignored_source_timestamp = 0;
    if (!decoder.Decode(ignored_source_timestamp)) {
      return std::nullopt;
    }
  }
  if ((mask & 0x08) != 0) {
    std::int64_t ignored_server_timestamp = 0;
    if (!decoder.Decode(ignored_server_timestamp)) {
      return std::nullopt;
    }
  }

  return value;
}

bool DecodeWriteValue(binary::BinaryDecoder& decoder,
                      scada::WriteValue& value) {
  std::uint32_t attribute_id = 0;
  std::string index_range;
  if (!decoder.Decode(value.node_id) ||
      !decoder.Decode(attribute_id) ||
      !decoder.Decode(index_range)) {
    return false;
  }

  auto decoded = DecodeDataValue(decoder);
  if (!decoded.has_value()) {
    return false;
  }

  value.attribute_id = static_cast<scada::AttributeId>(attribute_id);
  value.value = std::move(*decoded);
  return index_range.empty();
}

bool DecodeReadValueId(binary::BinaryDecoder& decoder,
                       scada::ReadValueId& value_id) {
  std::uint32_t attribute_id = 0;
  std::string index_range;
  scada::QualifiedName ignored_data_encoding;
  if (!decoder.Decode(value_id.node_id) ||
      !decoder.Decode(attribute_id) ||
      !decoder.Decode(index_range) ||
      !decoder.Decode(ignored_data_encoding)) {
    return false;
  }
  value_id.attribute_id = static_cast<scada::AttributeId>(attribute_id);
  return index_range.empty();
}

bool DecodeBrowseDescription(binary::BinaryDecoder& decoder,
                             scada::BrowseDescription& description) {
  std::uint32_t browse_direction = 0;
  std::uint32_t ignored_node_class_mask = 0;
  std::uint32_t ignored_result_mask = 0;
  bool include_subtypes = false;
  if (!decoder.Decode(description.node_id) ||
      !decoder.Decode(browse_direction) ||
      !decoder.Decode(description.reference_type_id) ||
      !decoder.Decode(include_subtypes) ||
      !decoder.Decode(ignored_node_class_mask) ||
      !decoder.Decode(ignored_result_mask)) {
    return false;
  }
  description.direction =
      static_cast<scada::BrowseDirection>(browse_direction);
  description.include_subtypes = include_subtypes;
  return description.direction == scada::BrowseDirection::Forward ||
         description.direction == scada::BrowseDirection::Inverse ||
         description.direction == scada::BrowseDirection::Both;
}

bool DecodeRelativePathElement(binary::BinaryDecoder& decoder,
                               scada::RelativePathElement& element) {
  return decoder.Decode(element.reference_type_id) &&
         decoder.Decode(element.inverse) &&
         decoder.Decode(element.include_subtypes) &&
         decoder.Decode(element.target_name);
}

bool DecodeBrowsePath(binary::BinaryDecoder& decoder, scada::BrowsePath& path) {
  std::int32_t count = 0;
  if (!decoder.Decode(path.node_id) || !decoder.Decode(count) || count < 0) {
    return false;
  }

  path.relative_path.resize(static_cast<std::size_t>(count));
  for (auto& element : path.relative_path) {
    if (!DecodeRelativePathElement(decoder, element)) {
      return false;
    }
  }
  return true;
}

bool DecodeDeleteNodesItem(binary::BinaryDecoder& decoder,
                           scada::DeleteNodesItem& item) {
  return decoder.Decode(item.node_id) &&
         decoder.Decode(item.delete_target_references);
}

bool DecodeAddNodesItem(binary::BinaryDecoder& decoder,
                        scada::AddNodesItem& item) {
  scada::ExpandedNodeId parent_id;
  scada::NodeId ignored_reference_type_id;
  scada::ExpandedNodeId requested_new_node_id;
  scada::QualifiedName browse_name;
  std::uint32_t node_class = 0;
  scada::ExpandedNodeId type_definition;
  if (!decoder.Decode(parent_id) ||
      !decoder.Decode(ignored_reference_type_id) ||
      !decoder.Decode(requested_new_node_id) ||
      !decoder.Decode(browse_name) || !decoder.Decode(node_class)) {
    return false;
  }

  item.parent_id = parent_id.node_id();
  item.requested_id = requested_new_node_id.node_id();
  item.node_class = static_cast<scada::NodeClass>(node_class);
  item.attributes.set_browse_name(std::move(browse_name));
  if (!DecodeNodeAttributesExtension(decoder, item.node_class, item.attributes) ||
      !decoder.Decode(type_definition)) {
    return false;
  }
  item.type_definition_id = type_definition.node_id();
  return true;
}

bool DecodeDeleteReferencesItem(binary::BinaryDecoder& decoder,
                                scada::DeleteReferencesItem& item) {
  return decoder.Decode(item.source_node_id) &&
         decoder.Decode(item.reference_type_id) &&
         decoder.Decode(item.forward) &&
         decoder.Decode(item.target_node_id) &&
         decoder.Decode(item.delete_bidirectional);
}

bool DecodeAddReferencesItem(binary::BinaryDecoder& decoder,
                             scada::AddReferencesItem& item) {
  std::uint32_t target_node_class = 0;
  if (!decoder.Decode(item.source_node_id) ||
      !decoder.Decode(item.reference_type_id) ||
      !decoder.Decode(item.forward) ||
      !decoder.Decode(item.target_server_uri) ||
      !decoder.Decode(item.target_node_id) ||
      !decoder.Decode(target_node_class)) {
    return false;
  }
  item.target_node_class = static_cast<scada::NodeClass>(target_node_class);
  return true;
}

std::optional<OpcUaBinaryDecodedRequest> DecodeCreateSessionRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  if (!ReadRequestHeader(decoder, header) ||
      !SkipApplicationDescriptionFields(decoder)) {
    return std::nullopt;
  }

  std::string ignored;
  scada::ByteString ignored_bytes;
  double requested_timeout_ms = 0;
  std::uint32_t ignored_max_response_size = 0;
  if (!decoder.Decode(ignored) || !decoder.Decode(ignored) ||
      !decoder.Decode(ignored) || !decoder.Decode(ignored_bytes) ||
      !decoder.Decode(ignored_bytes) ||
      !decoder.Decode(requested_timeout_ms) ||
      !decoder.Decode(ignored_max_response_size) || !decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = OpcUaCreateSessionRequest{
          .requested_timeout =
              base::TimeDelta::FromMillisecondsD(requested_timeout_ms)},
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeActivateSessionRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  if (!ReadRequestHeader(decoder, header) ||
      !header.authentication_token.is_numeric() ||
      !SkipSignatureData(decoder) ||
      !SkipSignedSoftwareCertificates(decoder) ||
      !SkipStringArray(decoder)) {
    return std::nullopt;
  }

  binary::DecodedExtensionObject user_identity;
  if (!decoder.Decode(user_identity) || user_identity.encoding != 0x01 ||
      !SkipSignatureData(decoder) || !decoder.consumed()) {
    return std::nullopt;
  }

  OpcUaActivateSessionRequest request{
      .authentication_token = header.authentication_token,
  };

  binary::BinaryDecoder body_decoder{user_identity.body};
  std::string ignored_policy_id;
  if (!body_decoder.Decode(ignored_policy_id)) {
    return std::nullopt;
  }
  if (user_identity.type_id == kAnonymousIdentityTokenBinaryEncodingId) {
    request.allow_anonymous = true;
  } else if (user_identity.type_id == kUserNameIdentityTokenBinaryEncodingId) {
    std::string user_name;
    scada::ByteString password;
    std::string encryption_algorithm;
    if (!body_decoder.Decode(user_name) ||
        !body_decoder.Decode(password) ||
        !body_decoder.Decode(encryption_algorithm) ||
        !body_decoder.consumed() ||
        !encryption_algorithm.empty()) {
      return std::nullopt;
    }
    request.user_name = scada::ToLocalizedText(user_name);
    request.password = scada::ToLocalizedText(
        std::string{password.begin(), password.end()});
  } else {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeCloseSessionRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::uint8_t delete_subscriptions = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(delete_subscriptions) || !decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = OpcUaCloseSessionRequest{
          .authentication_token = header.authentication_token,
      },
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeReadRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  double max_age = 0;
  std::uint32_t timestamps_to_return = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(max_age) ||
      !decoder.Decode(timestamps_to_return) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }
  if (max_age < 0) {
    return std::nullopt;
  }

  const auto timestamps =
      static_cast<OpcUaTimestampsToReturn>(timestamps_to_return);
  if (timestamps != OpcUaTimestampsToReturn::Source &&
      timestamps != OpcUaTimestampsToReturn::Server &&
      timestamps != OpcUaTimestampsToReturn::Both &&
      timestamps != OpcUaTimestampsToReturn::Neither) {
    return std::nullopt;
  }

  ReadRequest request;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeReadValueId(decoder, input)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeWriteRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  WriteRequest request;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeWriteValue(decoder, input)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeBrowseRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  scada::NodeId ignored_view_id;
  std::int64_t ignored_view_timestamp = 0;
  std::uint32_t ignored_view_version = 0;
  std::uint32_t requested_max_references_per_node = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(ignored_view_id) ||
      !decoder.Decode(ignored_view_timestamp) ||
      !decoder.Decode(ignored_view_version) ||
      !decoder.Decode(requested_max_references_per_node) ||
      !decoder.Decode(count) || count < 0) {
    return std::nullopt;
  }

  BrowseRequest request;
  request.requested_max_references_per_node = requested_max_references_per_node;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeBrowseDescription(decoder, input)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeCallRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  CallRequest request;
  request.methods.resize(static_cast<std::size_t>(count));
  for (auto& method : request.methods) {
    std::int32_t argument_count = 0;
    if (!decoder.Decode(method.object_id) ||
        !decoder.Decode(method.method_id) ||
        !decoder.Decode(argument_count) || argument_count < 0) {
      return std::nullopt;
    }

    method.arguments.reserve(static_cast<std::size_t>(argument_count));
    for (std::int32_t i = 0; i < argument_count; ++i) {
      scada::Variant argument;
      if (!decoder.Decode(argument)) {
        return std::nullopt;
      }
      method.arguments.push_back(std::move(argument));
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

bool DecodeLiteralOperandNodeId(const binary::DecodedExtensionObject& operand,
                                scada::NodeId& node_id) {
  if (operand.type_id != kLiteralOperandBinaryEncodingId ||
      operand.encoding != 0x01) {
    return false;
  }

  binary::BinaryDecoder decoder{operand.body};
  scada::Variant value;
  if (!decoder.Decode(value) || !decoder.consumed()) {
    return false;
  }

  auto* typed_node_id = value.get_if<scada::NodeId>();
  if (!typed_node_id) {
    return false;
  }
  node_id = *typed_node_id;
  return true;
}

bool DecodeEventFilterBody(binary::BinaryDecoder& filter_decoder,
                           scada::EventFilter& filter,
                           std::vector<std::vector<std::string>>& field_paths) {
  std::int32_t select_clause_count = 0;
  if (!filter_decoder.Decode(select_clause_count) || select_clause_count < 0) {
    return false;
  }

  field_paths.clear();
  field_paths.reserve(static_cast<std::size_t>(select_clause_count));
  for (std::int32_t i = 0; i < select_clause_count; ++i) {
    binary::DecodedExtensionObject operand;
    if (!filter_decoder.Decode(operand) ||
        operand.type_id != kSimpleAttributeOperandBinaryEncodingId ||
        operand.encoding != 0x01) {
      return false;
    }

    binary::BinaryDecoder operand_decoder{operand.body};
    scada::NodeId ignored_type_definition_id;
    std::int32_t browse_path_count = 0;
    std::uint32_t ignored_attribute_id = 0;
    std::string ignored_index_range;
    if (!operand_decoder.Decode(ignored_type_definition_id) ||
        !operand_decoder.Decode(browse_path_count) || browse_path_count < 0) {
      return false;
    }

    std::vector<std::string> browse_path;
    browse_path.reserve(static_cast<std::size_t>(browse_path_count));
    for (std::int32_t path_index = 0; path_index < browse_path_count;
         ++path_index) {
      scada::QualifiedName segment;
      if (!operand_decoder.Decode(segment)) {
        return false;
      }
      browse_path.emplace_back(segment.name());
    }

    if (!operand_decoder.Decode(ignored_attribute_id) ||
        !operand_decoder.Decode(ignored_index_range) ||
        !operand_decoder.consumed() || !ignored_index_range.empty()) {
      return false;
    }
    field_paths.push_back(std::move(browse_path));
  }

  std::int32_t where_clause_count = 0;
  if (!filter_decoder.Decode(where_clause_count) || where_clause_count < 0) {
    return false;
  }

  filter = {};
  for (std::int32_t i = 0; i < where_clause_count; ++i) {
    std::uint32_t filter_operator = 0;
    std::int32_t operand_count = 0;
    if (!filter_decoder.Decode(filter_operator) ||
        !filter_decoder.Decode(operand_count) || operand_count != 1) {
      return false;
    }

    binary::DecodedExtensionObject operand;
    scada::NodeId node_id;
    if (!filter_decoder.Decode(operand) ||
        !DecodeLiteralOperandNodeId(operand, node_id)) {
      return false;
    }

    if (filter_operator == static_cast<std::uint32_t>(OpcUa_FilterOperator_OfType)) {
      filter.of_type.push_back(std::move(node_id));
    } else if (filter_operator ==
               static_cast<std::uint32_t>(OpcUa_FilterOperator_RelatedTo)) {
      filter.child_of.push_back(std::move(node_id));
    } else {
      return false;
    }
  }

  return filter_decoder.consumed();
}

bool DecodeEventFilter(binary::BinaryDecoder& decoder,
                       scada::EventFilter& filter,
                       std::vector<std::vector<std::string>>& field_paths) {
  binary::DecodedExtensionObject encoded_filter;
  if (!decoder.Decode(encoded_filter)) {
    return false;
  }
  if (encoded_filter.type_id != kEventFilterBinaryEncodingId ||
      encoded_filter.encoding != 0x01) {
    return false;
  }

  binary::BinaryDecoder filter_decoder{encoded_filter.body};
  return DecodeEventFilterBody(filter_decoder, filter, field_paths);
}

bool DecodeMonitoringFilter(
    const binary::DecodedExtensionObject& encoded_filter,
    std::optional<OpcUaMonitoringFilter>& filter) {
  if (encoded_filter.type_id == 0 && encoded_filter.encoding == 0x00 &&
      encoded_filter.body.empty()) {
    filter.reset();
    return true;
  }
  if (encoded_filter.encoding != 0x01) {
    return false;
  }

  binary::BinaryDecoder decoder{encoded_filter.body};
  if (encoded_filter.type_id == kDataChangeFilterBinaryEncodingId) {
    OpcUaDataChangeFilter data_change_filter;
    std::uint32_t trigger = 0;
    std::uint32_t deadband_type = 0;
    if (!decoder.Decode(trigger) || !decoder.Decode(deadband_type) ||
        !decoder.Decode(data_change_filter.deadband_value) ||
        !decoder.consumed()) {
      return false;
    }
    data_change_filter.trigger =
        static_cast<OpcUaDataChangeTrigger>(trigger);
    data_change_filter.deadband_type =
        static_cast<OpcUaDeadbandType>(deadband_type);
    if (data_change_filter.trigger != OpcUaDataChangeTrigger::Status &&
        data_change_filter.trigger !=
            OpcUaDataChangeTrigger::StatusValue &&
        data_change_filter.trigger !=
            OpcUaDataChangeTrigger::StatusValueTimestamp) {
      return false;
    }
    if (data_change_filter.deadband_type != OpcUaDeadbandType::None &&
        data_change_filter.deadband_type != OpcUaDeadbandType::Absolute &&
        data_change_filter.deadband_type != OpcUaDeadbandType::Percent) {
      return false;
    }
    filter = std::move(data_change_filter);
    return true;
  }

  if (encoded_filter.type_id == kEventFilterBinaryEncodingId) {
    scada::EventFilter ignored_filter;
    std::vector<std::vector<std::string>> field_paths;
    if (!DecodeEventFilterBody(decoder, ignored_filter, field_paths)) {
      return false;
    }
    filter = OpcUaMonitoringFilter{
        scada::opcua_endpoint::BuildEventFilter(field_paths)};
    return true;
  }

  return false;
}

std::optional<OpcUaBinaryDecodedRequest> DecodeHistoryReadRawRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  binary::DecodedExtensionObject details;
  std::uint32_t timestamps_to_return = 0;
  bool release_continuation_points = false;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(details) ||
      !decoder.Decode(timestamps_to_return) ||
      !decoder.Decode(release_continuation_points) ||
      !decoder.Decode(count) || count != 1) {
    return std::nullopt;
  }

  const auto timestamps =
      static_cast<OpcUaTimestampsToReturn>(timestamps_to_return);
  if (timestamps != OpcUaTimestampsToReturn::Source &&
      timestamps != OpcUaTimestampsToReturn::Server &&
      timestamps != OpcUaTimestampsToReturn::Both &&
      timestamps != OpcUaTimestampsToReturn::Neither) {
    return std::nullopt;
  }

  if (details.type_id != kReadRawModifiedDetailsBinaryEncodingId ||
      details.encoding != 0x01) {
    return std::nullopt;
  }

  binary::BinaryDecoder details_decoder{details.body};
  bool is_read_modified = false;
  scada::DateTime from;
  scada::DateTime to;
  std::uint32_t max_count = 0;
  bool return_bounds = false;
  if (!details_decoder.Decode(is_read_modified) ||
      !details_decoder.Decode(from) ||
      !details_decoder.Decode(to) ||
      !details_decoder.Decode(max_count) ||
      !details_decoder.Decode(return_bounds) || !details_decoder.consumed() ||
      is_read_modified || return_bounds) {
    return std::nullopt;
  }

  HistoryReadRawRequest request;
  if (!decoder.Decode(request.details.node_id)) {
    return std::nullopt;
  }
  std::string index_range;
  scada::QualifiedName data_encoding;
  if (!decoder.Decode(index_range) || !decoder.Decode(data_encoding) ||
      !decoder.Decode(request.details.continuation_point) ||
      !decoder.consumed() || !index_range.empty() ||
      !data_encoding.name().empty() || data_encoding.namespace_index() != 0) {
    return std::nullopt;
  }

  request.details.from = from;
  request.details.to = to;
  request.details.max_count = max_count;
  request.details.release_continuation_point = release_continuation_points;

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeHistoryReadEventsRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  binary::DecodedExtensionObject details;
  std::uint32_t timestamps_to_return = 0;
  bool release_continuation_points = false;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(details) ||
      !decoder.Decode(timestamps_to_return) ||
      !decoder.Decode(release_continuation_points) ||
      !decoder.Decode(count) || count != 1) {
    return std::nullopt;
  }

  const auto timestamps =
      static_cast<OpcUaTimestampsToReturn>(timestamps_to_return);
  if (timestamps != OpcUaTimestampsToReturn::Source &&
      timestamps != OpcUaTimestampsToReturn::Server &&
      timestamps != OpcUaTimestampsToReturn::Both &&
      timestamps != OpcUaTimestampsToReturn::Neither) {
    return std::nullopt;
  }

  if (details.type_id != kReadEventDetailsBinaryEncodingId ||
      details.encoding != 0x01 || release_continuation_points) {
    return std::nullopt;
  }

  binary::BinaryDecoder details_decoder{details.body};
  std::uint32_t ignored_num_values_per_node = 0;
  HistoryReadEventsRequest request;
  std::vector<std::vector<std::string>> field_paths;
  if (!details_decoder.Decode(ignored_num_values_per_node) ||
      !details_decoder.Decode(request.details.from) ||
      !details_decoder.Decode(request.details.to) ||
      !DecodeEventFilter(details_decoder, request.details.filter, field_paths) ||
      !details_decoder.consumed()) {
    return std::nullopt;
  }

  if (!decoder.Decode(request.details.node_id)) {
    return std::nullopt;
  }
  std::string index_range;
  scada::QualifiedName data_encoding;
  scada::ByteString continuation_point;
  if (!decoder.Decode(index_range) || !decoder.Decode(data_encoding) ||
      !decoder.Decode(continuation_point) || !decoder.consumed() ||
      !index_range.empty() || !data_encoding.name().empty() ||
      data_encoding.namespace_index() != 0 || !continuation_point.empty()) {
    return std::nullopt;
  }

  field_paths =
      scada::opcua_endpoint::NormalizeEventFieldPaths(std::move(field_paths));

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
      .history_event_field_paths = std::move(field_paths),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeTranslateBrowsePathsRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  TranslateBrowsePathsRequest request;
  request.inputs.resize(static_cast<std::size_t>(count));
  for (auto& input : request.inputs) {
    if (!DecodeBrowsePath(decoder, input)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeBrowseNextRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  bool release_continuation_points = false;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(release_continuation_points) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  BrowseNextRequest request;
  request.release_continuation_points = release_continuation_points;
  request.continuation_points.resize(static_cast<std::size_t>(count));
  for (auto& continuation_point : request.continuation_points) {
    if (!decoder.Decode(continuation_point)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeCreateSubscriptionRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  OpcUaCreateSubscriptionRequest request;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(request.parameters.publishing_interval_ms) ||
      !decoder.Decode(request.parameters.lifetime_count) ||
      !decoder.Decode(request.parameters.max_keep_alive_count) ||
      !decoder.Decode(request.parameters.max_notifications_per_publish) ||
      !decoder.Decode(request.parameters.publishing_enabled) ||
      !decoder.Decode(request.parameters.priority) || !decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeDeleteSubscriptionsRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }
  OpcUaDeleteSubscriptionsRequest request;
  request.subscription_ids.resize(static_cast<std::size_t>(count));
  for (auto& subscription_id : request.subscription_ids) {
    if (!decoder.Decode(subscription_id)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeModifySubscriptionRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  OpcUaModifySubscriptionRequest request;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(request.subscription_id) ||
      !decoder.Decode(request.parameters.publishing_interval_ms) ||
      !decoder.Decode(request.parameters.lifetime_count) ||
      !decoder.Decode(request.parameters.max_keep_alive_count) ||
      !decoder.Decode(request.parameters.max_notifications_per_publish) ||
      !decoder.Decode(request.parameters.priority) || !decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeSetPublishingModeRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  OpcUaSetPublishingModeRequest request;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(request.publishing_enabled) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  request.subscription_ids.resize(static_cast<std::size_t>(count));
  for (auto& subscription_id : request.subscription_ids) {
    if (!decoder.Decode(subscription_id)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeCreateMonitoredItemsRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  OpcUaCreateMonitoredItemsRequest request;
  std::uint32_t timestamps_to_return = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(request.subscription_id) ||
      !decoder.Decode(timestamps_to_return) ||
      !decoder.Decode(count) || count < 0) {
    return std::nullopt;
  }

  request.timestamps_to_return =
      static_cast<OpcUaTimestampsToReturn>(timestamps_to_return);
  if (request.timestamps_to_return != OpcUaTimestampsToReturn::Source &&
      request.timestamps_to_return != OpcUaTimestampsToReturn::Server &&
      request.timestamps_to_return != OpcUaTimestampsToReturn::Both &&
      request.timestamps_to_return != OpcUaTimestampsToReturn::Neither) {
    return std::nullopt;
  }

  request.items_to_create.resize(static_cast<std::size_t>(count));
  for (auto& item : request.items_to_create) {
    std::uint32_t attribute_id = 0;
    std::string index_range;
    scada::QualifiedName ignored_data_encoding;
    std::uint32_t monitoring_mode = 0;
    binary::DecodedExtensionObject filter;
    if (!decoder.Decode(item.item_to_monitor.node_id) ||
        !decoder.Decode(attribute_id) || !decoder.Decode(index_range) ||
        !decoder.Decode(ignored_data_encoding) ||
        !decoder.Decode(monitoring_mode) ||
        !decoder.Decode(item.requested_parameters.client_handle) ||
        !decoder.Decode(item.requested_parameters.sampling_interval_ms) ||
        !decoder.Decode(filter) ||
        !decoder.Decode(item.requested_parameters.queue_size) ||
        !decoder.Decode(item.requested_parameters.discard_oldest)) {
      return std::nullopt;
    }
    item.item_to_monitor.attribute_id =
        static_cast<scada::AttributeId>(attribute_id);
    if (!index_range.empty()) {
      item.index_range = index_range;
    }
    item.monitoring_mode =
        static_cast<OpcUaMonitoringMode>(monitoring_mode);
    if (item.monitoring_mode != OpcUaMonitoringMode::Disabled &&
        item.monitoring_mode != OpcUaMonitoringMode::Sampling &&
        item.monitoring_mode != OpcUaMonitoringMode::Reporting) {
      return std::nullopt;
    }
    if (!DecodeMonitoringFilter(filter, item.requested_parameters.filter)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeModifyMonitoredItemsRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  OpcUaModifyMonitoredItemsRequest request;
  std::uint32_t timestamps_to_return = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(request.subscription_id) ||
      !decoder.Decode(timestamps_to_return) ||
      !decoder.Decode(count) || count < 0) {
    return std::nullopt;
  }

  request.timestamps_to_return =
      static_cast<OpcUaTimestampsToReturn>(timestamps_to_return);
  if (request.timestamps_to_return != OpcUaTimestampsToReturn::Source &&
      request.timestamps_to_return != OpcUaTimestampsToReturn::Server &&
      request.timestamps_to_return != OpcUaTimestampsToReturn::Both &&
      request.timestamps_to_return != OpcUaTimestampsToReturn::Neither) {
    return std::nullopt;
  }

  request.items_to_modify.resize(static_cast<std::size_t>(count));
  for (auto& item : request.items_to_modify) {
    binary::DecodedExtensionObject filter;
    if (!decoder.Decode(item.monitored_item_id) ||
        !decoder.Decode(item.requested_parameters.client_handle) ||
        !decoder.Decode(item.requested_parameters.sampling_interval_ms) ||
        !decoder.Decode(filter) ||
        !decoder.Decode(item.requested_parameters.queue_size) ||
        !decoder.Decode(item.requested_parameters.discard_oldest)) {
      return std::nullopt;
    }
    if (!DecodeMonitoringFilter(filter, item.requested_parameters.filter)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodePublishRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  OpcUaPublishRequest request;
  request.subscription_acknowledgements.resize(static_cast<std::size_t>(count));
  for (auto& acknowledgement : request.subscription_acknowledgements) {
    if (!decoder.Decode(acknowledgement.subscription_id) ||
        !decoder.Decode(acknowledgement.sequence_number)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeTransferSubscriptionsRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  OpcUaTransferSubscriptionsRequest request;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  request.subscription_ids.resize(static_cast<std::size_t>(count));
  for (auto& subscription_id : request.subscription_ids) {
    if (!decoder.Decode(subscription_id)) {
      return std::nullopt;
    }
  }
  if (!decoder.Decode(request.send_initial_values) || !decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeDeleteMonitoredItemsRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  OpcUaDeleteMonitoredItemsRequest request;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(request.subscription_id) ||
      !decoder.Decode(count) || count < 0) {
    return std::nullopt;
  }
  request.monitored_item_ids.resize(static_cast<std::size_t>(count));
  for (auto& monitored_item_id : request.monitored_item_ids) {
    if (!decoder.Decode(monitored_item_id)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeSetMonitoringModeRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  OpcUaSetMonitoringModeRequest request;
  std::uint32_t monitoring_mode = 0;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(request.subscription_id) ||
      !decoder.Decode(monitoring_mode) ||
      !decoder.Decode(count) || count < 0) {
    return std::nullopt;
  }
  request.monitoring_mode =
      static_cast<OpcUaMonitoringMode>(monitoring_mode);
  if (request.monitoring_mode != OpcUaMonitoringMode::Disabled &&
      request.monitoring_mode != OpcUaMonitoringMode::Sampling &&
      request.monitoring_mode != OpcUaMonitoringMode::Reporting) {
    return std::nullopt;
  }
  request.monitored_item_ids.resize(static_cast<std::size_t>(count));
  for (auto& monitored_item_id : request.monitored_item_ids) {
    if (!decoder.Decode(monitored_item_id)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeRepublishRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  OpcUaRepublishRequest request;
  if (!ReadRequestHeader(decoder, header) ||
      !decoder.Decode(request.subscription_id) ||
      !decoder.Decode(request.retransmit_sequence_number) ||
      !decoder.consumed()) {
    return std::nullopt;
  }
  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeDeleteNodesRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  DeleteNodesRequest request;
  request.items.resize(static_cast<std::size_t>(count));
  for (auto& item : request.items) {
    if (!DecodeDeleteNodesItem(decoder, item)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeAddNodesRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  AddNodesRequest request;
  request.items.resize(static_cast<std::size_t>(count));
  for (auto& item : request.items) {
    if (!DecodeAddNodesItem(decoder, item)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeDeleteReferencesRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  DeleteReferencesRequest request;
  request.items.resize(static_cast<std::size_t>(count));
  for (auto& item : request.items) {
    if (!DecodeDeleteReferencesItem(decoder, item)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

std::optional<OpcUaBinaryDecodedRequest> DecodeAddReferencesRequest(
    std::span<const char> body) {
  binary::BinaryDecoder decoder{body};
  OpcUaBinaryServiceRequestHeader header;
  std::int32_t count = 0;
  if (!ReadRequestHeader(decoder, header) || !decoder.Decode(count) ||
      count < 0) {
    return std::nullopt;
  }

  AddReferencesRequest request;
  request.items.resize(static_cast<std::size_t>(count));
  for (auto& item : request.items) {
    if (!DecodeAddReferencesItem(decoder, item)) {
      return std::nullopt;
    }
  }
  if (!decoder.consumed()) {
    return std::nullopt;
  }

  return OpcUaBinaryDecodedRequest{
      .header = header,
      .body = std::move(request),
  };
}

}  // namespace

std::optional<std::vector<char>> EncodeOpcUaBinaryServiceRequest(
    const OpcUaBinaryServiceRequestHeader& header,
    const OpcUaRequestBody& request) {
  return std::visit(
      [&](const auto& typed_request) -> std::optional<std::vector<char>> {
        std::vector<char> payload;
        std::vector<char> body;
        binary::BinaryEncoder payload_encoder{payload};
        binary::BinaryEncoder body_encoder{body};

        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, OpcUaCreateSessionRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(std::uint8_t{0});
          payload_encoder.Encode(std::int32_t{0});
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(std::int32_t{-1});
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(std::string_view{"opc.tcp://localhost:4840"});
          payload_encoder.Encode(std::string_view{"binary-session"});
          payload_encoder.Encode(scada::ByteString{});
          payload_encoder.Encode(scada::ByteString{});
          payload_encoder.Encode(
              typed_request.requested_timeout.InMillisecondsF());
          payload_encoder.Encode(std::uint32_t{0});
          binary::AppendMessage(
              body_encoder, kCreateSessionRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaActivateSessionRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(scada::ByteString{});
          payload_encoder.Encode(std::int32_t{-1});
          payload_encoder.Encode(std::int32_t{-1});
          if (typed_request.allow_anonymous) {
            std::vector<char> identity;
            binary::BinaryEncoder identity_encoder{identity};
            identity_encoder.Encode(std::string_view{""});
            payload_encoder.Encode(binary::EncodedExtensionObject{
                .type_id = kAnonymousIdentityTokenBinaryEncodingId,
                .body = identity});
          } else {
            std::vector<char> identity;
            binary::BinaryEncoder identity_encoder{identity};
            identity_encoder.Encode(std::string_view{""});
            identity_encoder.Encode(
                typed_request.user_name.has_value()
                    ? ToString(*typed_request.user_name)
                    : std::string{});
            const auto password =
                typed_request.password.has_value()
                    ? ToString(*typed_request.password)
                    : std::string{};
            identity_encoder.Encode(
                scada::ByteString{password.begin(), password.end()});
            identity_encoder.Encode(std::string_view{""});
            payload_encoder.Encode(binary::EncodedExtensionObject{
                .type_id = kUserNameIdentityTokenBinaryEncodingId,
                .body = identity});
          }
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(scada::ByteString{});
          binary::AppendMessage(
              body_encoder, kActivateSessionRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, OpcUaCloseSessionRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(std::uint8_t{1});
          binary::AppendMessage(
              body_encoder, kCloseSessionRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaCreateSubscriptionRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(typed_request.parameters.publishing_interval_ms);
          payload_encoder.Encode(typed_request.parameters.lifetime_count);
          payload_encoder.Encode(typed_request.parameters.max_keep_alive_count);
          payload_encoder.Encode(
              typed_request.parameters.max_notifications_per_publish);
          payload_encoder.Encode(typed_request.parameters.publishing_enabled);
          payload_encoder.Encode(typed_request.parameters.priority);
          binary::AppendMessage(
              body_encoder, kCreateSubscriptionRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaModifySubscriptionRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(typed_request.subscription_id);
          payload_encoder.Encode(typed_request.parameters.publishing_interval_ms);
          payload_encoder.Encode(typed_request.parameters.lifetime_count);
          payload_encoder.Encode(typed_request.parameters.max_keep_alive_count);
          payload_encoder.Encode(
              typed_request.parameters.max_notifications_per_publish);
          payload_encoder.Encode(typed_request.parameters.priority);
          binary::AppendMessage(
              body_encoder, kModifySubscriptionRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaSetPublishingModeRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(typed_request.publishing_enabled);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.subscription_ids.size()));
          for (const auto subscription_id : typed_request.subscription_ids) {
            payload_encoder.Encode(subscription_id);
          }
          binary::AppendMessage(
              body_encoder, kSetPublishingModeRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaDeleteSubscriptionsRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.subscription_ids.size()));
          for (const auto subscription_id : typed_request.subscription_ids) {
            payload_encoder.Encode(subscription_id);
          }
          binary::AppendMessage(
              body_encoder, kDeleteSubscriptionsRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaCreateMonitoredItemsRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(typed_request.subscription_id);
          payload_encoder.Encode(
              static_cast<std::uint32_t>(typed_request.timestamps_to_return));
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.items_to_create.size()));
          for (const auto& item : typed_request.items_to_create) {
            AppendMonitoredItemCreateRequest(payload_encoder, item);
          }
          binary::AppendMessage(
              body_encoder, kCreateMonitoredItemsRequestBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaModifyMonitoredItemsRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(typed_request.subscription_id);
          payload_encoder.Encode(
              static_cast<std::uint32_t>(typed_request.timestamps_to_return));
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.items_to_modify.size()));
          for (const auto& item : typed_request.items_to_modify) {
            payload_encoder.Encode(item.monitored_item_id);
            payload_encoder.Encode(item.requested_parameters.client_handle);
            payload_encoder.Encode(
                item.requested_parameters.sampling_interval_ms);
            AppendMonitoringFilter(payload_encoder,
                                   item.requested_parameters.filter);
            payload_encoder.Encode(item.requested_parameters.queue_size);
            payload_encoder.Encode(item.requested_parameters.discard_oldest);
          }
          binary::AppendMessage(
              body_encoder, kModifyMonitoredItemsRequestBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<T, OpcUaPublishRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(static_cast<std::int32_t>(
              typed_request.subscription_acknowledgements.size()));
          for (const auto& acknowledgement :
               typed_request.subscription_acknowledgements) {
            payload_encoder.Encode(acknowledgement.subscription_id);
            payload_encoder.Encode(acknowledgement.sequence_number);
          }
          binary::AppendMessage(body_encoder, kPublishRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, OpcUaRepublishRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(typed_request.subscription_id);
          payload_encoder.Encode(typed_request.retransmit_sequence_number);
          binary::AppendMessage(body_encoder, kRepublishRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaTransferSubscriptionsRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.subscription_ids.size()));
          for (const auto subscription_id : typed_request.subscription_ids) {
            payload_encoder.Encode(subscription_id);
          }
          payload_encoder.Encode(typed_request.send_initial_values);
          binary::AppendMessage(
              body_encoder, kTransferSubscriptionsRequestBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaDeleteMonitoredItemsRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(typed_request.subscription_id);
          payload_encoder.Encode(static_cast<std::int32_t>(
              typed_request.monitored_item_ids.size()));
          for (const auto monitored_item_id : typed_request.monitored_item_ids) {
            payload_encoder.Encode(monitored_item_id);
          }
          binary::AppendMessage(
              body_encoder, kDeleteMonitoredItemsRequestBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaSetMonitoringModeRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(typed_request.subscription_id);
          payload_encoder.Encode(
              static_cast<std::uint32_t>(typed_request.monitoring_mode));
          payload_encoder.Encode(static_cast<std::int32_t>(
              typed_request.monitored_item_ids.size()));
          for (const auto monitored_item_id : typed_request.monitored_item_ids) {
            payload_encoder.Encode(monitored_item_id);
          }
          binary::AppendMessage(
              body_encoder, kSetMonitoringModeRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, ReadRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(0.0);
          payload_encoder.Encode(
              static_cast<std::uint32_t>(WireTimestampsToReturn::Both));
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.inputs.size()));
          for (const auto& input : typed_request.inputs) {
            payload_encoder.Encode(input.node_id);
            payload_encoder.Encode(
                static_cast<std::uint32_t>(input.attribute_id));
            payload_encoder.Encode(std::string_view{""});
            payload_encoder.Encode(std::uint16_t{0});
            payload_encoder.Encode(std::string_view{""});
          }
          binary::AppendMessage(body_encoder, kReadRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, WriteRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.inputs.size()));
          for (const auto& input : typed_request.inputs) {
            payload_encoder.Encode(input.node_id);
            payload_encoder.Encode(
                static_cast<std::uint32_t>(input.attribute_id));
            payload_encoder.Encode(std::string_view{""});
            payload_encoder.Encode(std::uint8_t{0x01});
            payload_encoder.Encode(input.value);
          }
          binary::AppendMessage(body_encoder, kWriteRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, BrowseRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(scada::NodeId{});
          payload_encoder.Encode(std::int64_t{0});
          payload_encoder.Encode(std::uint32_t{0});
          payload_encoder.Encode(
              typed_request.requested_max_references_per_node);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.inputs.size()));
          for (const auto& input : typed_request.inputs) {
            payload_encoder.Encode(input.node_id);
            payload_encoder.Encode(
                static_cast<std::uint32_t>(input.direction));
            payload_encoder.Encode(input.reference_type_id);
            payload_encoder.Encode(input.include_subtypes);
            payload_encoder.Encode(std::uint32_t{0});
            payload_encoder.Encode(std::uint32_t{0});
          }
          binary::AppendMessage(body_encoder, kBrowseRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, BrowseNextRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(typed_request.release_continuation_points);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.continuation_points.size()));
          for (const auto& continuation_point : typed_request.continuation_points) {
            payload_encoder.Encode(continuation_point);
          }
          binary::AppendMessage(body_encoder, kBrowseNextRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T,
                                            TranslateBrowsePathsRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.inputs.size()));
          for (const auto& input : typed_request.inputs) {
            AppendBrowsePath(payload_encoder, input);
          }
          binary::AppendMessage(
              body_encoder, kTranslateBrowsePathsRequestBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<T, CallRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.methods.size()));
          for (const auto& method : typed_request.methods) {
            payload_encoder.Encode(method.object_id);
            payload_encoder.Encode(method.method_id);
            payload_encoder.Encode(
                static_cast<std::int32_t>(method.arguments.size()));
            for (const auto& argument : method.arguments) {
              payload_encoder.Encode(argument);
            }
          }
          binary::AppendMessage(body_encoder, kCallRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, HistoryReadRawRequest>) {
          AppendRequestHeader(payload_encoder, header);

          std::vector<char> details;
          binary::BinaryEncoder details_encoder{details};
          details_encoder.Encode(false);
          details_encoder.Encode(typed_request.details.from);
          details_encoder.Encode(typed_request.details.to);
          details_encoder.Encode(
              static_cast<std::uint32_t>(typed_request.details.max_count));
          details_encoder.Encode(false);

          payload_encoder.Encode(binary::EncodedExtensionObject{
              .type_id = kReadRawModifiedDetailsBinaryEncodingId,
              .body = std::move(details),
          });
          payload_encoder.Encode(
              static_cast<std::uint32_t>(WireTimestampsToReturn::Both));
          payload_encoder.Encode(
              typed_request.details.release_continuation_point);
          payload_encoder.Encode(std::int32_t{1});
          payload_encoder.Encode(typed_request.details.node_id);
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(scada::QualifiedName{});
          payload_encoder.Encode(typed_request.details.continuation_point);
          binary::AppendMessage(
              body_encoder, kHistoryReadRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, HistoryReadEventsRequest>) {
          AppendRequestHeader(payload_encoder, header);

          std::vector<char> details;
          binary::BinaryEncoder details_encoder{details};
          details_encoder.Encode(std::uint32_t{0});
          details_encoder.Encode(typed_request.details.from);
          details_encoder.Encode(typed_request.details.to);
          const auto& field_paths = scada::opcua_endpoint::DefaultEventFieldPaths();
          AppendEventFilter(details_encoder, field_paths, typed_request.details.filter);

          payload_encoder.Encode(binary::EncodedExtensionObject{
              .type_id = kReadEventDetailsBinaryEncodingId,
              .body = std::move(details),
          });
          payload_encoder.Encode(
              static_cast<std::uint32_t>(WireTimestampsToReturn::Both));
          payload_encoder.Encode(false);
          payload_encoder.Encode(std::int32_t{1});
          payload_encoder.Encode(typed_request.details.node_id);
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(scada::QualifiedName{});
          payload_encoder.Encode(scada::ByteString{});
          binary::AppendMessage(
              body_encoder, kHistoryReadRequestBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, AddNodesRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.items.size()));
          for (const auto& item : typed_request.items) {
            const auto encoded_attributes =
                EncodeNodeAttributesExtension(item.node_class, item.attributes);
            if (!encoded_attributes.has_value()) {
              return std::nullopt;
            }
            payload_encoder.Encode(scada::ExpandedNodeId{item.parent_id});
            payload_encoder.Encode(scada::NodeId{});
            payload_encoder.Encode(scada::ExpandedNodeId{item.requested_id});
            payload_encoder.Encode(item.attributes.browse_name);
            payload_encoder.Encode(static_cast<std::uint32_t>(item.node_class));
            payload_encoder.Encode(*encoded_attributes);
            payload_encoder.Encode(
                scada::ExpandedNodeId{item.type_definition_id});
          }
          binary::AppendMessage(body_encoder, kAddNodesRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, DeleteNodesRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.items.size()));
          for (const auto& item : typed_request.items) {
            payload_encoder.Encode(item.node_id);
            payload_encoder.Encode(item.delete_target_references);
          }
          binary::AppendMessage(body_encoder, kDeleteNodesRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T,
                                            DeleteReferencesRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.items.size()));
          for (const auto& item : typed_request.items) {
            payload_encoder.Encode(item.source_node_id);
            payload_encoder.Encode(item.reference_type_id);
            payload_encoder.Encode(item.forward);
            payload_encoder.Encode(item.target_node_id);
            payload_encoder.Encode(item.delete_bidirectional);
          }
          binary::AppendMessage(body_encoder,
                                kDeleteReferencesRequestBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, AddReferencesRequest>) {
          AppendRequestHeader(payload_encoder, header);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_request.items.size()));
          for (const auto& item : typed_request.items) {
            payload_encoder.Encode(item.source_node_id);
            payload_encoder.Encode(item.reference_type_id);
            payload_encoder.Encode(item.forward);
            payload_encoder.Encode(item.target_server_uri);
            payload_encoder.Encode(item.target_node_id);
            payload_encoder.Encode(
                static_cast<std::uint32_t>(item.target_node_class));
          }
          binary::AppendMessage(body_encoder, kAddReferencesRequestBinaryEncodingId,
                                payload);
        } else {
          return std::nullopt;
        }

        return body;
      },
      request);
}

std::optional<OpcUaBinaryDecodedRequest> DecodeOpcUaBinaryServiceRequest(
    const std::vector<char>& payload) {
  binary::BinaryDecoder decoder{payload};
  const auto message = binary::ReadMessage(decoder);
  if (!message.has_value()) {
    return std::nullopt;
  }

  switch (message->first) {
    case kCreateSessionRequestBinaryEncodingId:
      return DecodeCreateSessionRequest(message->second);
    case kActivateSessionRequestBinaryEncodingId:
      return DecodeActivateSessionRequest(message->second);
    case kCloseSessionRequestBinaryEncodingId:
      return DecodeCloseSessionRequest(message->second);
    case kCreateSubscriptionRequestBinaryEncodingId:
      return DecodeCreateSubscriptionRequest(message->second);
    case kModifySubscriptionRequestBinaryEncodingId:
      return DecodeModifySubscriptionRequest(message->second);
    case kSetPublishingModeRequestBinaryEncodingId:
      return DecodeSetPublishingModeRequest(message->second);
    case kDeleteSubscriptionsRequestBinaryEncodingId:
      return DecodeDeleteSubscriptionsRequest(message->second);
    case kCreateMonitoredItemsRequestBinaryEncodingId:
      return DecodeCreateMonitoredItemsRequest(message->second);
    case kModifyMonitoredItemsRequestBinaryEncodingId:
      return DecodeModifyMonitoredItemsRequest(message->second);
    case kPublishRequestBinaryEncodingId:
      return DecodePublishRequest(message->second);
    case kRepublishRequestBinaryEncodingId:
      return DecodeRepublishRequest(message->second);
    case kTransferSubscriptionsRequestBinaryEncodingId:
      return DecodeTransferSubscriptionsRequest(message->second);
    case kDeleteMonitoredItemsRequestBinaryEncodingId:
      return DecodeDeleteMonitoredItemsRequest(message->second);
    case kSetMonitoringModeRequestBinaryEncodingId:
      return DecodeSetMonitoringModeRequest(message->second);
    case kBrowseRequestBinaryEncodingId:
      return DecodeBrowseRequest(message->second);
    case kBrowseNextRequestBinaryEncodingId:
      return DecodeBrowseNextRequest(message->second);
    case kTranslateBrowsePathsRequestBinaryEncodingId:
      return DecodeTranslateBrowsePathsRequest(message->second);
    case kCallRequestBinaryEncodingId:
      return DecodeCallRequest(message->second);
    case kHistoryReadRequestBinaryEncodingId:
      if (const auto decoded = DecodeHistoryReadRawRequest(message->second)) {
        return decoded;
      }
      return DecodeHistoryReadEventsRequest(message->second);
    case kReadRequestBinaryEncodingId:
      return DecodeReadRequest(message->second);
    case kWriteRequestBinaryEncodingId:
      return DecodeWriteRequest(message->second);
    case kAddNodesRequestBinaryEncodingId:
      return DecodeAddNodesRequest(message->second);
    case kDeleteNodesRequestBinaryEncodingId:
      return DecodeDeleteNodesRequest(message->second);
    case kDeleteReferencesRequestBinaryEncodingId:
      return DecodeDeleteReferencesRequest(message->second);
    case kAddReferencesRequestBinaryEncodingId:
      return DecodeAddReferencesRequest(message->second);
    default:
      return std::nullopt;
  }
}

std::optional<std::vector<char>> EncodeOpcUaBinaryServiceResponse(
    std::uint32_t request_handle,
    const OpcUaResponseBody& response) {
  return std::visit(
      [&](const auto& typed_response) -> std::optional<std::vector<char>> {
        std::vector<char> payload;
        std::vector<char> body;
        binary::BinaryEncoder payload_encoder{payload};
        binary::BinaryEncoder body_encoder{body};

        using T = std::decay_t<decltype(typed_response)>;
        if constexpr (std::is_same_v<T, OpcUaCreateSessionResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(typed_response.session_id);
          payload_encoder.Encode(typed_response.authentication_token);
          payload_encoder.Encode(
              typed_response.revised_timeout.InMillisecondsF());
          payload_encoder.Encode(typed_response.server_nonce);
          payload_encoder.Encode(scada::ByteString{});
          payload_encoder.Encode(std::int32_t{-1});
          payload_encoder.Encode(std::int32_t{-1});
          payload_encoder.Encode(std::string_view{""});
          payload_encoder.Encode(scada::ByteString{});
          payload_encoder.Encode(std::uint32_t{0});
          binary::AppendMessage(
              body_encoder, kCreateSessionResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaActivateSessionResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(scada::ByteString{});
          payload_encoder.Encode(std::int32_t{-1});
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kActivateSessionResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaCloseSessionResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          binary::AppendMessage(
              body_encoder, kCloseSessionResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaCreateSubscriptionResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(typed_response.subscription_id);
          payload_encoder.Encode(typed_response.revised_publishing_interval_ms);
          payload_encoder.Encode(typed_response.revised_lifetime_count);
          payload_encoder.Encode(typed_response.revised_max_keep_alive_count);
          binary::AppendMessage(
              body_encoder, kCreateSubscriptionResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaModifySubscriptionResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(typed_response.revised_publishing_interval_ms);
          payload_encoder.Encode(typed_response.revised_lifetime_count);
          payload_encoder.Encode(typed_response.revised_max_keep_alive_count);
          binary::AppendMessage(
              body_encoder, kModifySubscriptionResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaSetPublishingModeResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kSetPublishingModeResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T,
                                            OpcUaDeleteSubscriptionsResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kDeleteSubscriptionsResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaCreateMonitoredItemsResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            AppendMonitoredItemCreateResult(payload_encoder, result);
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kCreateMonitoredItemsResponseBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaModifyMonitoredItemsResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            AppendMonitoredItemModifyResult(payload_encoder, result);
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kModifyMonitoredItemsResponseBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<T, OpcUaPublishResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(typed_response.subscription_id);
          payload_encoder.Encode(static_cast<std::int32_t>(
              typed_response.available_sequence_numbers.size()));
          for (const auto sequence_number :
               typed_response.available_sequence_numbers) {
            payload_encoder.Encode(sequence_number);
          }
          payload_encoder.Encode(typed_response.more_notifications);
          AppendNotificationMessage(payload_encoder,
                                    typed_response.notification_message);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(body_encoder, kPublishResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, OpcUaRepublishResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          AppendNotificationMessage(payload_encoder,
                                    typed_response.notification_message);
          binary::AppendMessage(body_encoder, kRepublishResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaTransferSubscriptionsResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kTransferSubscriptionsResponseBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaDeleteMonitoredItemsResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kDeleteMonitoredItemsResponseBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<
                                 T, OpcUaSetMonitoringModeResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kSetMonitoringModeResponseBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<T, ReadResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            AppendDataValue(payload_encoder, result);
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(body_encoder, kReadResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, WriteResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(body_encoder, kWriteResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, BrowseResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result.status_code));
            payload_encoder.Encode(result.continuation_point);
            payload_encoder.Encode(
                static_cast<std::int32_t>(result.references.size()));
            for (const auto& reference : result.references) {
              AppendReferenceDescription(payload_encoder, reference);
            }
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(body_encoder, kBrowseResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, BrowseNextResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result.status_code));
            payload_encoder.Encode(result.continuation_point);
            payload_encoder.Encode(
                static_cast<std::int32_t>(result.references.size()));
            for (const auto& reference : result.references) {
              AppendReferenceDescription(payload_encoder, reference);
            }
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(body_encoder, kBrowseNextResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<
                                 T, TranslateBrowsePathsResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            AppendBrowsePathResult(payload_encoder, result);
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kTranslateBrowsePathsResponseBinaryEncodingId,
              payload);
        } else if constexpr (std::is_same_v<T, CallResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               scada::StatusCode::Good);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.Encode(result.status.full_code());
            payload_encoder.Encode(static_cast<std::int32_t>(
                result.input_argument_results.size()));
            for (const auto& input_result : result.input_argument_results) {
              payload_encoder.Encode(EncodeStatusCode(input_result));
            }
            payload_encoder.Encode(std::int32_t{-1});
            payload_encoder.Encode(
                static_cast<std::int32_t>(result.output_arguments.size()));
            for (const auto& output_argument : result.output_arguments) {
              payload_encoder.Encode(output_argument);
            }
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(body_encoder, kCallResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, HistoryReadRawResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.result.status);
          payload_encoder.Encode(std::int32_t{1});
          payload_encoder.Encode(typed_response.result.status.full_code());
          payload_encoder.Encode(typed_response.result.continuation_point);
          AppendHistoryData(payload_encoder, typed_response.result);
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(body_encoder, kHistoryReadResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, AddNodesResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result.status_code));
            payload_encoder.Encode(result.added_node_id);
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(body_encoder, kAddNodesResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<T, DeleteNodesResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(body_encoder, kDeleteNodesResponseBinaryEncodingId,
                                payload);
        } else if constexpr (std::is_same_v<
                                 T, DeleteReferencesResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kDeleteReferencesResponseBinaryEncodingId, payload);
        } else if constexpr (std::is_same_v<T, AddReferencesResponse>) {
          AppendResponseHeader(payload_encoder, request_handle,
                               typed_response.status);
          payload_encoder.Encode(
              static_cast<std::int32_t>(typed_response.results.size()));
          for (const auto& result : typed_response.results) {
            payload_encoder.Encode(EncodeStatusCode(result));
          }
          payload_encoder.Encode(std::int32_t{-1});
          binary::AppendMessage(
              body_encoder, kAddReferencesResponseBinaryEncodingId, payload);
        } else {
          return std::nullopt;
        }

        return body;
      },
      response);
}

std::optional<std::vector<char>> EncodeOpcUaBinaryHistoryReadEventsResponse(
    std::uint32_t request_handle,
    const HistoryReadEventsResponse& response,
    std::span<const std::vector<std::string>> field_paths) {
  std::vector<char> payload;
  std::vector<char> body;
  binary::BinaryEncoder payload_encoder{payload};
  binary::BinaryEncoder body_encoder{body};

  AppendResponseHeader(payload_encoder, request_handle, response.result.status);
  payload_encoder.Encode(std::int32_t{1});
  payload_encoder.Encode(response.result.status.full_code());
  payload_encoder.Encode(scada::ByteString{});
  AppendHistoryEvent(payload_encoder, response, field_paths);
  payload_encoder.Encode(std::int32_t{-1});
  binary::AppendMessage(body_encoder, kHistoryReadResponseBinaryEncodingId, payload);
  return body;
}

}  // namespace opcua
