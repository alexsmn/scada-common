#include "opcua/websocket/json_codec.h"

#include <boost/json.hpp>

#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace opcua::detail {

namespace {

using boost::json::array;
using boost::json::object;
using boost::json::string;
using boost::json::value;

[[noreturn]] void ThrowJsonError(std::string_view message) {
  throw std::runtime_error(std::string{message});
}

const object& RequireObject(const value& json) {
  if (!json.is_object())
    ThrowJsonError("Expected JSON object");
  return json.as_object();
}

const array& RequireArray(const value& json) {
  if (!json.is_array())
    ThrowJsonError("Expected JSON array");
  return json.as_array();
}

const value& RequireField(const object& json, std::string_view key) {
  if (const auto* field = json.if_contains(key))
    return *field;
  ThrowJsonError("Missing required field");
}

const value* FindField(const object& json, std::string_view key) {
  return json.if_contains(key);
}

std::string_view RequireString(const value& json) {
  if (!json.is_string())
    ThrowJsonError("Expected JSON string");
  return json.as_string();
}

bool RequireBool(const value& json) {
  if (!json.is_bool())
    ThrowJsonError("Expected JSON bool");
  return json.as_bool();
}

std::uint64_t RequireUInt64(const value& json) {
  if (json.is_uint64())
    return json.as_uint64();
  if (json.is_int64() && json.as_int64() >= 0)
    return static_cast<std::uint64_t>(json.as_int64());
  ThrowJsonError("Expected JSON unsigned integer");
}

double RequireDouble(const value& json) {
  if (json.is_double())
    return json.as_double();
  if (json.is_int64())
    return static_cast<double>(json.as_int64());
  if (json.is_uint64())
    return static_cast<double>(json.as_uint64());
  ThrowJsonError("Expected JSON number");
}

value EncodeNodeId(const scada::NodeId& node_id) {
  return string(node_id.ToString());
}

scada::NodeId DecodeNodeId(const value& json) {
  const auto text = RequireString(json);
  auto node_id = scada::NodeId::FromString(text);
  if (node_id.is_null() && text != "i=0")
    ThrowJsonError("Invalid NodeId");
  return node_id;
}

value EncodeStatus(const scada::Status& status) {
  return static_cast<std::uint64_t>(status.full_code());
}

scada::Status DecodeStatus(const value& json) {
  if (json.is_uint64() || (json.is_int64() && json.as_int64() >= 0)) {
    return scada::Status::FromFullCode(
        static_cast<unsigned>(RequireUInt64(json)));
  }
  const auto& obj = RequireObject(json);
  return scada::Status::FromFullCode(static_cast<unsigned>(
      RequireUInt64(RequireField(obj, "fullCode"))));
}

value EncodeStatusCode(scada::StatusCode status_code) {
  return static_cast<std::uint64_t>(static_cast<unsigned>(status_code));
}

scada::StatusCode DecodeStatusCode(const value& json) {
  return static_cast<scada::StatusCode>(
      static_cast<unsigned>(RequireUInt64(json)));
}

value EncodeAttributeId(scada::AttributeId attribute_id) {
  return static_cast<std::uint64_t>(static_cast<unsigned>(attribute_id));
}

scada::AttributeId DecodeAttributeId(const value& json) {
  return static_cast<scada::AttributeId>(
      static_cast<unsigned>(RequireUInt64(json)));
}

template <class T, class Encoder>
array EncodeList(const std::vector<T>& values, Encoder&& encoder) {
  array result;
  result.reserve(values.size());
  for (const auto& entry : values)
    result.emplace_back(encoder(entry));
  return result;
}

template <class T, class Decoder>
std::vector<T> DecodeList(const value& json, Decoder&& decoder) {
  std::vector<T> result;
  for (const auto& entry : RequireArray(json))
    result.emplace_back(decoder(entry));
  return result;
}

template <class Enum>
value EncodeEnum(Enum value) {
  return static_cast<std::uint64_t>(
      static_cast<std::underlying_type_t<Enum>>(value));
}

template <class Enum>
Enum DecodeEnum(const value& json) {
  return static_cast<Enum>(
      static_cast<std::underlying_type_t<Enum>>(RequireUInt64(json)));
}

value EncodeReadValueId(const scada::ReadValueId& value_id) {
  return object{{"NodeId", EncodeNodeId(value_id.node_id)},
                {"AttributeId", EncodeAttributeId(value_id.attribute_id)}};
}

scada::ReadValueId DecodeReadValueId(const value& json) {
  const auto& obj = RequireObject(json);
  return {.node_id = DecodeNodeId(RequireField(obj, "NodeId")),
          .attribute_id = DecodeAttributeId(RequireField(obj, "AttributeId"))};
}

value EncodeDataChangeFilter(const opcua::OpcUaDataChangeFilter& filter) {
  return object{{"FilterType", "DataChange"},
                {"Trigger", EncodeEnum(filter.trigger)},
                {"DeadbandType", EncodeEnum(filter.deadband_type)},
                {"DeadbandValue", filter.deadband_value}};
}

opcua::OpcUaDataChangeFilter DecodeDataChangeFilter(const value& json) {
  const auto& obj = RequireObject(json);
  return {.trigger = DecodeEnum<opcua::OpcUaDataChangeTrigger>(
              RequireField(obj, "Trigger")),
          .deadband_type = DecodeEnum<opcua::OpcUaDeadbandType>(
              RequireField(obj, "DeadbandType")),
          .deadband_value = RequireDouble(RequireField(obj, "DeadbandValue"))};
}

value EncodeMonitoringFilter(const opcua::OpcUaMonitoringFilter& filter) {
  if (const auto* data_change = std::get_if<opcua::OpcUaDataChangeFilter>(&filter))
    return EncodeDataChangeFilter(*data_change);
  return std::get<boost::json::value>(filter);
}

opcua::OpcUaMonitoringFilter DecodeMonitoringFilter(const value& json) {
  if (json.is_object()) {
    const auto& obj = json.as_object();
    if (const auto* field = FindField(obj, "FilterType");
        field != nullptr && RequireString(*field) == "DataChange") {
      return opcua::OpcUaMonitoringFilter{DecodeDataChangeFilter(json)};
    }
  }
  return opcua::OpcUaMonitoringFilter{json};
}

value EncodeMonitoringParameters(
    const opcua::OpcUaMonitoringParameters& parameters) {
  object json{{"ClientHandle", parameters.client_handle},
              {"SamplingInterval", parameters.sampling_interval_ms},
              {"QueueSize", parameters.queue_size},
              {"DiscardOldest", parameters.discard_oldest}};
  if (parameters.filter.has_value())
    json["Filter"] = EncodeMonitoringFilter(*parameters.filter);
  return json;
}

opcua::OpcUaMonitoringParameters DecodeMonitoringParameters(const value& json) {
  const auto& obj = RequireObject(json);
  opcua::OpcUaMonitoringParameters parameters{
      .client_handle =
          static_cast<scada::UInt32>(RequireUInt64(RequireField(obj, "ClientHandle"))),
      .sampling_interval_ms =
          RequireDouble(RequireField(obj, "SamplingInterval")),
      .queue_size =
          static_cast<scada::UInt32>(RequireUInt64(RequireField(obj, "QueueSize"))),
      .discard_oldest = RequireBool(RequireField(obj, "DiscardOldest")),
  };
  if (const auto* field = FindField(obj, "Filter"))
    parameters.filter = DecodeMonitoringFilter(*field);
  return parameters;
}

value EncodeMonitoredItemCreateRequest(
    const opcua::OpcUaMonitoredItemCreateRequest& request) {
  object json{{"ItemToMonitor", EncodeReadValueId(request.item_to_monitor)},
              {"MonitoringMode", EncodeEnum(request.monitoring_mode)},
              {"RequestedParameters",
               EncodeMonitoringParameters(request.requested_parameters)}};
  if (request.index_range.has_value())
    json["IndexRange"] = *request.index_range;
  return json;
}

opcua::OpcUaMonitoredItemCreateRequest DecodeMonitoredItemCreateRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  opcua::OpcUaMonitoredItemCreateRequest request{
      .item_to_monitor = DecodeReadValueId(RequireField(obj, "ItemToMonitor")),
      .monitoring_mode =
          DecodeEnum<opcua::OpcUaMonitoringMode>(RequireField(obj, "MonitoringMode")),
      .requested_parameters =
          DecodeMonitoringParameters(RequireField(obj, "RequestedParameters")),
  };
  if (const auto* field = FindField(obj, "IndexRange"))
    request.index_range = std::string(RequireString(*field));
  return request;
}

value EncodeMonitoredItemCreateResult(
    const opcua::OpcUaMonitoredItemCreateResult& result) {
  object json{{"StatusCode", EncodeStatusCode(result.status.code())},
              {"MonitoredItemId", result.monitored_item_id},
              {"RevisedSamplingInterval", result.revised_sampling_interval_ms},
              {"RevisedQueueSize", result.revised_queue_size}};
  if (result.filter_result.has_value())
    json["FilterResult"] = *result.filter_result;
  return json;
}

opcua::OpcUaMonitoredItemCreateResult DecodeMonitoredItemCreateResult(
    const value& json) {
  const auto& obj = RequireObject(json);
  const auto* status_field = FindField(obj, "StatusCode");
  const auto* legacy_status_field = FindField(obj, "Status");
  if (!status_field && !legacy_status_field)
    ThrowJsonError("Missing required field");
  opcua::OpcUaMonitoredItemCreateResult result{
      .status = status_field ? scada::Status{DecodeStatusCode(*status_field)}
                             : DecodeStatus(*legacy_status_field),
      .monitored_item_id = static_cast<opcua::OpcUaMonitoredItemId>(
          RequireUInt64(RequireField(obj, "MonitoredItemId"))),
      .revised_sampling_interval_ms =
          RequireDouble(RequireField(obj, "RevisedSamplingInterval")),
      .revised_queue_size = static_cast<scada::UInt32>(
          RequireUInt64(RequireField(obj, "RevisedQueueSize"))),
  };
  if (const auto* field = FindField(obj, "FilterResult"))
    result.filter_result = *field;
  return result;
}

value EncodeMonitoredItemModifyRequest(
    const opcua::OpcUaMonitoredItemModifyRequest& request) {
  return object{
      {"MonitoredItemId", request.monitored_item_id},
      {"RequestedParameters",
       EncodeMonitoringParameters(request.requested_parameters)}};
}

opcua::OpcUaMonitoredItemModifyRequest DecodeMonitoredItemModifyRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.monitored_item_id = static_cast<opcua::OpcUaMonitoredItemId>(
              RequireUInt64(RequireField(obj, "MonitoredItemId"))),
          .requested_parameters =
              DecodeMonitoringParameters(RequireField(obj, "RequestedParameters"))};
}

value EncodeMonitoredItemModifyResult(
    const opcua::OpcUaMonitoredItemModifyResult& result) {
  object json{{"StatusCode", EncodeStatusCode(result.status.code())},
              {"RevisedSamplingInterval", result.revised_sampling_interval_ms},
              {"RevisedQueueSize", result.revised_queue_size}};
  if (result.filter_result.has_value())
    json["FilterResult"] = *result.filter_result;
  return json;
}

opcua::OpcUaMonitoredItemModifyResult DecodeMonitoredItemModifyResult(
    const value& json) {
  const auto& obj = RequireObject(json);
  const auto* status_field = FindField(obj, "StatusCode");
  const auto* legacy_status_field = FindField(obj, "Status");
  if (!status_field && !legacy_status_field)
    ThrowJsonError("Missing required field");
  opcua::OpcUaMonitoredItemModifyResult result{
      .status = status_field ? scada::Status{DecodeStatusCode(*status_field)}
                             : DecodeStatus(*legacy_status_field),
      .revised_sampling_interval_ms =
          RequireDouble(RequireField(obj, "RevisedSamplingInterval")),
      .revised_queue_size = static_cast<scada::UInt32>(
          RequireUInt64(RequireField(obj, "RevisedQueueSize"))),
  };
  if (const auto* field = FindField(obj, "FilterResult"))
    result.filter_result = *field;
  return result;
}

value EncodeSubscriptionParameters(
    const opcua::OpcUaSubscriptionParameters& parameters) {
  return object{{"PublishingInterval", parameters.publishing_interval_ms},
                {"LifetimeCount", parameters.lifetime_count},
                {"MaxKeepAliveCount", parameters.max_keep_alive_count},
                {"MaxNotificationsPerPublish",
                 parameters.max_notifications_per_publish},
                {"PublishingEnabled", parameters.publishing_enabled},
                {"Priority", parameters.priority}};
}

opcua::OpcUaSubscriptionParameters DecodeSubscriptionParameters(const value& json) {
  const auto& obj = RequireObject(json);
  return {.publishing_interval_ms =
              RequireDouble(RequireField(obj, "PublishingInterval")),
          .lifetime_count = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "LifetimeCount"))),
          .max_keep_alive_count = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "MaxKeepAliveCount"))),
          .max_notifications_per_publish = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "MaxNotificationsPerPublish"))),
          .publishing_enabled =
              RequireBool(RequireField(obj, "PublishingEnabled")),
          .priority =
              static_cast<scada::UInt8>(RequireUInt64(RequireField(obj, "Priority")))};
}

template <class Response>
value EncodeMultiStatusResponse(const Response& response) {
  return object{{"Status", EncodeStatus(response.status)},
                {"Results", EncodeList(response.results, EncodeStatusCode)}};
}

template <class Response>
Response DecodeMultiStatusResponse(const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "Status")),
          .results = DecodeList<scada::StatusCode>(
              RequireField(obj, "Results"), DecodeStatusCode)};
}

}  // namespace

value EncodeCreateSubscriptionRequest(
    const opcua::OpcUaCreateSubscriptionRequest& request) {
  return object{{"RequestedParameters",
                 EncodeSubscriptionParameters(request.parameters)}};
}

opcua::OpcUaCreateSubscriptionRequest DecodeCreateSubscriptionRequest(
    const value& json) {
  return {.parameters =
              DecodeSubscriptionParameters(
                  RequireField(RequireObject(json), "RequestedParameters"))};
}

value EncodeCreateSubscriptionResponse(
    const opcua::OpcUaCreateSubscriptionResponse& response) {
  return object{
      {"Status", EncodeStatus(response.status)},
      {"SubscriptionId", response.subscription_id},
      {"RevisedPublishingInterval", response.revised_publishing_interval_ms},
      {"RevisedLifetimeCount", response.revised_lifetime_count},
      {"RevisedMaxKeepAliveCount", response.revised_max_keep_alive_count}};
}

opcua::OpcUaCreateSubscriptionResponse DecodeCreateSubscriptionResponse(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "Status")),
          .subscription_id = static_cast<opcua::OpcUaSubscriptionId>(
              RequireUInt64(RequireField(obj, "SubscriptionId"))),
          .revised_publishing_interval_ms =
              RequireDouble(RequireField(obj, "RevisedPublishingInterval")),
          .revised_lifetime_count = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "RevisedLifetimeCount"))),
          .revised_max_keep_alive_count = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "RevisedMaxKeepAliveCount")))};
}

value EncodeModifySubscriptionRequest(
    const opcua::OpcUaModifySubscriptionRequest& request) {
  return object{{"SubscriptionId", request.subscription_id},
                {"RequestedParameters",
                 EncodeSubscriptionParameters(request.parameters)}};
}

opcua::OpcUaModifySubscriptionRequest DecodeModifySubscriptionRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.subscription_id = static_cast<opcua::OpcUaSubscriptionId>(
              RequireUInt64(RequireField(obj, "SubscriptionId"))),
          .parameters =
              DecodeSubscriptionParameters(RequireField(obj, "RequestedParameters"))};
}

value EncodeModifySubscriptionResponse(
    const opcua::OpcUaModifySubscriptionResponse& response) {
  return object{
      {"Status", EncodeStatus(response.status)},
      {"RevisedPublishingInterval", response.revised_publishing_interval_ms},
      {"RevisedLifetimeCount", response.revised_lifetime_count},
      {"RevisedMaxKeepAliveCount", response.revised_max_keep_alive_count}};
}

opcua::OpcUaModifySubscriptionResponse DecodeModifySubscriptionResponse(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "Status")),
          .revised_publishing_interval_ms =
              RequireDouble(RequireField(obj, "RevisedPublishingInterval")),
          .revised_lifetime_count = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "RevisedLifetimeCount"))),
          .revised_max_keep_alive_count = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "RevisedMaxKeepAliveCount")))};
}

value EncodeSetPublishingModeRequest(
    const opcua::OpcUaSetPublishingModeRequest& request) {
  return object{{"PublishingEnabled", request.publishing_enabled},
                {"SubscriptionIds",
                 EncodeList(request.subscription_ids, [](auto id) {
                   return value(static_cast<std::uint64_t>(id));
                 })}};
}

opcua::OpcUaSetPublishingModeRequest DecodeSetPublishingModeRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.publishing_enabled =
              RequireBool(RequireField(obj, "PublishingEnabled")),
          .subscription_ids = DecodeList<opcua::OpcUaSubscriptionId>(
              RequireField(obj, "SubscriptionIds"), [](const value& entry) {
                return static_cast<opcua::OpcUaSubscriptionId>(RequireUInt64(entry));
              })};
}

value EncodeSetPublishingModeResponse(
    const opcua::OpcUaSetPublishingModeResponse& response) {
  return EncodeMultiStatusResponse(response);
}

opcua::OpcUaSetPublishingModeResponse DecodeSetPublishingModeResponse(
    const value& json) {
  return DecodeMultiStatusResponse<opcua::OpcUaSetPublishingModeResponse>(json);
}

value EncodeDeleteSubscriptionsRequest(
    const opcua::OpcUaDeleteSubscriptionsRequest& request) {
  return object{{"SubscriptionIds",
                 EncodeList(request.subscription_ids, [](auto id) {
                   return value(static_cast<std::uint64_t>(id));
                 })}};
}

opcua::OpcUaDeleteSubscriptionsRequest DecodeDeleteSubscriptionsRequest(
    const value& json) {
  return {.subscription_ids = DecodeList<opcua::OpcUaSubscriptionId>(
              RequireField(RequireObject(json), "SubscriptionIds"),
              [](const value& entry) {
                return static_cast<opcua::OpcUaSubscriptionId>(RequireUInt64(entry));
              })};
}

value EncodeDeleteSubscriptionsResponse(
    const opcua::OpcUaDeleteSubscriptionsResponse& response) {
  return EncodeMultiStatusResponse(response);
}

opcua::OpcUaDeleteSubscriptionsResponse DecodeDeleteSubscriptionsResponse(
    const value& json) {
  return DecodeMultiStatusResponse<opcua::OpcUaDeleteSubscriptionsResponse>(json);
}

value EncodeCreateMonitoredItemsRequest(
    const opcua::OpcUaCreateMonitoredItemsRequest& request) {
  return object{
      {"SubscriptionId", request.subscription_id},
      {"TimestampsToReturn", EncodeEnum(request.timestamps_to_return)},
      {"ItemsToCreate",
       EncodeList(request.items_to_create, EncodeMonitoredItemCreateRequest)}};
}

opcua::OpcUaCreateMonitoredItemsRequest DecodeCreateMonitoredItemsRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.subscription_id = static_cast<opcua::OpcUaSubscriptionId>(
              RequireUInt64(RequireField(obj, "SubscriptionId"))),
          .timestamps_to_return = DecodeEnum<opcua::OpcUaTimestampsToReturn>(
              RequireField(obj, "TimestampsToReturn")),
          .items_to_create = DecodeList<opcua::OpcUaMonitoredItemCreateRequest>(
              RequireField(obj, "ItemsToCreate"),
              DecodeMonitoredItemCreateRequest)};
}

value EncodeCreateMonitoredItemsResponse(
    const opcua::OpcUaCreateMonitoredItemsResponse& response) {
  return object{{"Status", EncodeStatus(response.status)},
                {"Results",
                 EncodeList(response.results, EncodeMonitoredItemCreateResult)}};
}

opcua::OpcUaCreateMonitoredItemsResponse DecodeCreateMonitoredItemsResponse(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "Status")),
          .results = DecodeList<opcua::OpcUaMonitoredItemCreateResult>(
              RequireField(obj, "Results"), DecodeMonitoredItemCreateResult)};
}

value EncodeModifyMonitoredItemsRequest(
    const opcua::OpcUaModifyMonitoredItemsRequest& request) {
  return object{
      {"SubscriptionId", request.subscription_id},
      {"TimestampsToReturn", EncodeEnum(request.timestamps_to_return)},
      {"ItemsToModify",
       EncodeList(request.items_to_modify, EncodeMonitoredItemModifyRequest)}};
}

opcua::OpcUaModifyMonitoredItemsRequest DecodeModifyMonitoredItemsRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.subscription_id = static_cast<opcua::OpcUaSubscriptionId>(
              RequireUInt64(RequireField(obj, "SubscriptionId"))),
          .timestamps_to_return = DecodeEnum<opcua::OpcUaTimestampsToReturn>(
              RequireField(obj, "TimestampsToReturn")),
          .items_to_modify = DecodeList<opcua::OpcUaMonitoredItemModifyRequest>(
              RequireField(obj, "ItemsToModify"),
              DecodeMonitoredItemModifyRequest)};
}

value EncodeModifyMonitoredItemsResponse(
    const opcua::OpcUaModifyMonitoredItemsResponse& response) {
  return object{{"Status", EncodeStatus(response.status)},
                {"Results",
                 EncodeList(response.results, EncodeMonitoredItemModifyResult)}};
}

opcua::OpcUaModifyMonitoredItemsResponse DecodeModifyMonitoredItemsResponse(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "Status")),
          .results = DecodeList<opcua::OpcUaMonitoredItemModifyResult>(
              RequireField(obj, "Results"), DecodeMonitoredItemModifyResult)};
}

value EncodeDeleteMonitoredItemsRequest(
    const opcua::OpcUaDeleteMonitoredItemsRequest& request) {
  return object{{"SubscriptionId", request.subscription_id},
                {"MonitoredItemIds",
                 EncodeList(request.monitored_item_ids, [](auto id) {
                   return value(static_cast<std::uint64_t>(id));
                 })}};
}

opcua::OpcUaDeleteMonitoredItemsRequest DecodeDeleteMonitoredItemsRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.subscription_id = static_cast<opcua::OpcUaSubscriptionId>(
              RequireUInt64(RequireField(obj, "SubscriptionId"))),
          .monitored_item_ids = DecodeList<opcua::OpcUaMonitoredItemId>(
              RequireField(obj, "MonitoredItemIds"), [](const value& entry) {
                return static_cast<opcua::OpcUaMonitoredItemId>(RequireUInt64(entry));
              })};
}

value EncodeDeleteMonitoredItemsResponse(
    const opcua::OpcUaDeleteMonitoredItemsResponse& response) {
  return EncodeMultiStatusResponse(response);
}

opcua::OpcUaDeleteMonitoredItemsResponse DecodeDeleteMonitoredItemsResponse(
    const value& json) {
  return DecodeMultiStatusResponse<opcua::OpcUaDeleteMonitoredItemsResponse>(json);
}

value EncodeSetMonitoringModeRequest(
    const opcua::OpcUaSetMonitoringModeRequest& request) {
  return object{{"SubscriptionId", request.subscription_id},
                {"MonitoringMode", EncodeEnum(request.monitoring_mode)},
                {"MonitoredItemIds",
                 EncodeList(request.monitored_item_ids, [](auto id) {
                   return value(static_cast<std::uint64_t>(id));
                 })}};
}

opcua::OpcUaSetMonitoringModeRequest DecodeSetMonitoringModeRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.subscription_id = static_cast<opcua::OpcUaSubscriptionId>(
              RequireUInt64(RequireField(obj, "SubscriptionId"))),
          .monitoring_mode = DecodeEnum<opcua::OpcUaMonitoringMode>(
              RequireField(obj, "MonitoringMode")),
          .monitored_item_ids = DecodeList<opcua::OpcUaMonitoredItemId>(
              RequireField(obj, "MonitoredItemIds"), [](const value& entry) {
                return static_cast<opcua::OpcUaMonitoredItemId>(RequireUInt64(entry));
              })};
}

value EncodeSetMonitoringModeResponse(
    const opcua::OpcUaSetMonitoringModeResponse& response) {
  return EncodeMultiStatusResponse(response);
}

opcua::OpcUaSetMonitoringModeResponse DecodeSetMonitoringModeResponse(
    const value& json) {
  return DecodeMultiStatusResponse<opcua::OpcUaSetMonitoringModeResponse>(json);
}

}  // namespace opcua::detail
