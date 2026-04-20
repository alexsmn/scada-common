#include "opcua_ws/opcua_json_codec.h"

#include "base/time_utils.h"

#include <boost/json.hpp>

#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace opcua_ws::detail {

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

value EncodeStatus(const scada::Status& status) {
  return object{{"fullCode", status.full_code()}};
}

scada::Status DecodeStatus(const value& json) {
  const auto& obj = RequireObject(json);
  return scada::Status::FromFullCode(
      static_cast<unsigned>(RequireUInt64(RequireField(obj, "fullCode"))));
}

value EncodeStatusCode(scada::StatusCode status_code) {
  return static_cast<std::uint64_t>(static_cast<unsigned>(status_code));
}

scada::StatusCode DecodeStatusCode(const value& json) {
  return static_cast<scada::StatusCode>(
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

value EncodeDateTime(scada::DateTime time) {
  return string(SerializeToString(time));
}

scada::DateTime DecodeDateTime(const value& json) {
  scada::DateTime result;
  if (!Deserialize(RequireString(json), result))
    ThrowJsonError("Invalid DateTime");
  return result;
}

value EncodeVariant(const scada::Variant& variant) {
  CallRequest request{
      .methods = {{.object_id = scada::NodeId{},
                   .method_id = scada::NodeId{},
                   .arguments = {variant}}}};
  const auto service_json = RequireObject(EncodeJson(OpcUaWsServiceRequest{request}));
  const auto& body = RequireObject(RequireField(service_json, "body"));
  const auto& methods = RequireArray(RequireField(body, "methods"));
  return RequireArray(RequireField(RequireObject(methods.front()), "arguments"))
      .front();
}

scada::Variant DecodeVariant(const value& json) {
  const object wrapper{
      {"service", "Call"},
      {"body",
       object{{"methods",
               array{object{{"objectId", "i=0"},
                            {"methodId", "i=0"},
                            {"arguments", array{json}}}}}}}};
  const auto request = std::get<CallRequest>(DecodeServiceRequest(wrapper));
  return request.methods.front().arguments.front();
}

value EncodeDataValue(const scada::DataValue& data_value) {
  ReadResponse response{.status = scada::StatusCode::Good,
                        .results = {data_value}};
  const auto service_json =
      RequireObject(EncodeJson(OpcUaWsServiceResponse{response}));
  const auto& body = RequireObject(RequireField(service_json, "body"));
  return RequireArray(RequireField(body, "results")).front();
}

scada::DataValue DecodeDataValue(const value& json) {
  const object wrapper{
      {"service", "Read"},
      {"body",
       object{{"status", object{{"fullCode", 0u}}},
              {"results", array{json}}}}};
  const auto response = std::get<ReadResponse>(DecodeServiceResponse(wrapper));
  return response.results.front();
}

value EncodeSubscriptionAcknowledgement(
    const OpcUaWsSubscriptionAcknowledgement& acknowledgement) {
  return object{{"subscriptionId", acknowledgement.subscription_id},
                {"sequenceNumber", acknowledgement.sequence_number}};
}

OpcUaWsSubscriptionAcknowledgement DecodeSubscriptionAcknowledgement(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.subscription_id = static_cast<OpcUaWsSubscriptionId>(
              RequireUInt64(RequireField(obj, "subscriptionId"))),
          .sequence_number = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "sequenceNumber")))};
}

value EncodeMonitoredItemNotification(
    const OpcUaWsMonitoredItemNotification& notification) {
  return object{{"clientHandle", notification.client_handle},
                {"value", EncodeDataValue(notification.value)}};
}

OpcUaWsMonitoredItemNotification DecodeMonitoredItemNotification(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.client_handle = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "clientHandle"))),
          .value = DecodeDataValue(RequireField(obj, "value"))};
}

value EncodeEventFieldList(const OpcUaWsEventFieldList& fields) {
  return object{{"clientHandle", fields.client_handle},
                {"eventFields", EncodeList(fields.event_fields, EncodeVariant)}};
}

OpcUaWsEventFieldList DecodeEventFieldList(const value& json) {
  const auto& obj = RequireObject(json);
  return {.client_handle = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "clientHandle"))),
          .event_fields = DecodeList<scada::Variant>(
              RequireField(obj, "eventFields"), DecodeVariant)};
}

value EncodeNotificationData(const OpcUaWsNotificationData& notification) {
  return std::visit(
      [](const auto& typed_notification) -> value {
        using T = std::decay_t<decltype(typed_notification)>;
        if constexpr (std::is_same_v<T, OpcUaWsDataChangeNotification>) {
          return object{
              {"type", "DataChangeNotification"},
              {"monitoredItems",
               EncodeList(typed_notification.monitored_items,
                          EncodeMonitoredItemNotification)}};
        } else if constexpr (std::is_same_v<T, OpcUaWsEventNotificationList>) {
          return object{{"type", "EventNotificationList"},
                        {"events", EncodeList(typed_notification.events,
                                              EncodeEventFieldList)}};
        } else {
          return object{{"type", "StatusChangeNotification"},
                        {"status", EncodeStatusCode(typed_notification.status)}};
        }
      },
      notification);
}

OpcUaWsNotificationData DecodeNotificationData(const value& json) {
  const auto& obj = RequireObject(json);
  const auto type = RequireString(RequireField(obj, "type"));
  if (type == "DataChangeNotification") {
    return OpcUaWsDataChangeNotification{
        .monitored_items = DecodeList<OpcUaWsMonitoredItemNotification>(
            RequireField(obj, "monitoredItems"), DecodeMonitoredItemNotification)};
  }
  if (type == "EventNotificationList") {
    return OpcUaWsEventNotificationList{
        .events = DecodeList<OpcUaWsEventFieldList>(
            RequireField(obj, "events"), DecodeEventFieldList)};
  }
  if (type == "StatusChangeNotification") {
    return OpcUaWsStatusChangeNotification{
        .status = DecodeStatusCode(RequireField(obj, "status"))};
  }
  ThrowJsonError("Unknown NotificationData type");
}

value EncodeNotificationMessage(
    const OpcUaWsNotificationMessage& notification_message) {
  return object{{"sequenceNumber", notification_message.sequence_number},
                {"publishTime", EncodeDateTime(notification_message.publish_time)},
                {"notificationData",
                 EncodeList(notification_message.notification_data,
                            EncodeNotificationData)}};
}

OpcUaWsNotificationMessage DecodeNotificationMessage(const value& json) {
  const auto& obj = RequireObject(json);
  return {.sequence_number = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "sequenceNumber"))),
          .publish_time = DecodeDateTime(RequireField(obj, "publishTime")),
          .notification_data = DecodeList<OpcUaWsNotificationData>(
              RequireField(obj, "notificationData"), DecodeNotificationData)};
}

template <class Response>
value EncodeMultiStatusResponse(const Response& response) {
  return object{{"status", EncodeStatus(response.status)},
                {"results", EncodeList(response.results, EncodeStatusCode)}};
}

template <class Response>
Response DecodeMultiStatusResponse(const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "status")),
          .results = DecodeList<scada::StatusCode>(
              RequireField(obj, "results"), DecodeStatusCode)};
}

}  // namespace

value EncodePublishRequest(const OpcUaWsPublishRequest& request) {
  return object{{"subscriptionAcknowledgements",
                 EncodeList(request.subscription_acknowledgements,
                            EncodeSubscriptionAcknowledgement)}};
}

OpcUaWsPublishRequest DecodePublishRequest(const value& json) {
  return {.subscription_acknowledgements =
              DecodeList<OpcUaWsSubscriptionAcknowledgement>(
                  RequireField(RequireObject(json), "subscriptionAcknowledgements"),
                  DecodeSubscriptionAcknowledgement)};
}

value EncodePublishResponse(const OpcUaWsPublishResponse& response) {
  return object{{"status", EncodeStatus(response.status)},
                {"subscriptionId", response.subscription_id},
                {"availableSequenceNumbers",
                 EncodeList(response.available_sequence_numbers, [](auto value) {
                   return boost::json::value(
                       static_cast<std::uint64_t>(value));
                 })},
                {"moreNotifications", response.more_notifications},
                {"notificationMessage",
                 EncodeNotificationMessage(response.notification_message)},
                {"results", EncodeList(response.results, EncodeStatusCode)}};
}

OpcUaWsPublishResponse DecodePublishResponse(const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "status")),
          .subscription_id = static_cast<OpcUaWsSubscriptionId>(
              RequireUInt64(RequireField(obj, "subscriptionId"))),
          .available_sequence_numbers = DecodeList<scada::UInt32>(
              RequireField(obj, "availableSequenceNumbers"),
              [](const value& entry) {
                return static_cast<scada::UInt32>(RequireUInt64(entry));
              }),
          .more_notifications =
              RequireBool(RequireField(obj, "moreNotifications")),
          .notification_message =
              DecodeNotificationMessage(RequireField(obj, "notificationMessage")),
          .results = DecodeList<scada::StatusCode>(
              RequireField(obj, "results"), DecodeStatusCode)};
}

value EncodeRepublishRequest(const OpcUaWsRepublishRequest& request) {
  return object{{"subscriptionId", request.subscription_id},
                {"retransmitSequenceNumber",
                 request.retransmit_sequence_number}};
}

OpcUaWsRepublishRequest DecodeRepublishRequest(const value& json) {
  const auto& obj = RequireObject(json);
  return {.subscription_id = static_cast<OpcUaWsSubscriptionId>(
              RequireUInt64(RequireField(obj, "subscriptionId"))),
          .retransmit_sequence_number = static_cast<scada::UInt32>(
              RequireUInt64(RequireField(obj, "retransmitSequenceNumber")))};
}

value EncodeRepublishResponse(const OpcUaWsRepublishResponse& response) {
  return object{{"status", EncodeStatus(response.status)},
                {"notificationMessage",
                 EncodeNotificationMessage(response.notification_message)}};
}

OpcUaWsRepublishResponse DecodeRepublishResponse(const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "status")),
          .notification_message =
              DecodeNotificationMessage(RequireField(obj, "notificationMessage"))};
}

value EncodeTransferSubscriptionsRequest(
    const OpcUaWsTransferSubscriptionsRequest& request) {
  return object{{"subscriptionIds",
                 EncodeList(request.subscription_ids, [](auto value) {
                   return boost::json::value(
                       static_cast<std::uint64_t>(value));
                 })},
                {"sendInitialValues", request.send_initial_values}};
}

OpcUaWsTransferSubscriptionsRequest DecodeTransferSubscriptionsRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.subscription_ids = DecodeList<OpcUaWsSubscriptionId>(
              RequireField(obj, "subscriptionIds"), [](const value& entry) {
                return static_cast<OpcUaWsSubscriptionId>(RequireUInt64(entry));
              }),
          .send_initial_values =
              RequireBool(RequireField(obj, "sendInitialValues"))};
}

value EncodeTransferSubscriptionsResponse(
    const OpcUaWsTransferSubscriptionsResponse& response) {
  return EncodeMultiStatusResponse(response);
}

OpcUaWsTransferSubscriptionsResponse DecodeTransferSubscriptionsResponse(
    const value& json) {
  return DecodeMultiStatusResponse<OpcUaWsTransferSubscriptionsResponse>(json);
}

}  // namespace opcua_ws::detail
