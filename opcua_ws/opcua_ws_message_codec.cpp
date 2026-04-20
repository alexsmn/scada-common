#include "opcua_ws/opcua_json_codec.h"

#include "base/utf_convert.h"

#include <boost/json.hpp>

#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace opcua_ws {

namespace detail {
boost::json::value EncodeCreateSubscriptionRequest(
    const OpcUaWsCreateSubscriptionRequest& request);
OpcUaWsCreateSubscriptionRequest DecodeCreateSubscriptionRequest(
    const boost::json::value& json);
boost::json::value EncodeCreateSubscriptionResponse(
    const OpcUaWsCreateSubscriptionResponse& response);
OpcUaWsCreateSubscriptionResponse DecodeCreateSubscriptionResponse(
    const boost::json::value& json);
boost::json::value EncodeModifySubscriptionRequest(
    const OpcUaWsModifySubscriptionRequest& request);
OpcUaWsModifySubscriptionRequest DecodeModifySubscriptionRequest(
    const boost::json::value& json);
boost::json::value EncodeModifySubscriptionResponse(
    const OpcUaWsModifySubscriptionResponse& response);
OpcUaWsModifySubscriptionResponse DecodeModifySubscriptionResponse(
    const boost::json::value& json);
boost::json::value EncodeSetPublishingModeRequest(
    const OpcUaWsSetPublishingModeRequest& request);
OpcUaWsSetPublishingModeRequest DecodeSetPublishingModeRequest(
    const boost::json::value& json);
boost::json::value EncodeSetPublishingModeResponse(
    const OpcUaWsSetPublishingModeResponse& response);
OpcUaWsSetPublishingModeResponse DecodeSetPublishingModeResponse(
    const boost::json::value& json);
boost::json::value EncodeDeleteSubscriptionsRequest(
    const OpcUaWsDeleteSubscriptionsRequest& request);
OpcUaWsDeleteSubscriptionsRequest DecodeDeleteSubscriptionsRequest(
    const boost::json::value& json);
boost::json::value EncodeDeleteSubscriptionsResponse(
    const OpcUaWsDeleteSubscriptionsResponse& response);
OpcUaWsDeleteSubscriptionsResponse DecodeDeleteSubscriptionsResponse(
    const boost::json::value& json);
boost::json::value EncodeCreateMonitoredItemsRequest(
    const OpcUaWsCreateMonitoredItemsRequest& request);
OpcUaWsCreateMonitoredItemsRequest DecodeCreateMonitoredItemsRequest(
    const boost::json::value& json);
boost::json::value EncodeCreateMonitoredItemsResponse(
    const OpcUaWsCreateMonitoredItemsResponse& response);
OpcUaWsCreateMonitoredItemsResponse DecodeCreateMonitoredItemsResponse(
    const boost::json::value& json);
boost::json::value EncodeModifyMonitoredItemsRequest(
    const OpcUaWsModifyMonitoredItemsRequest& request);
OpcUaWsModifyMonitoredItemsRequest DecodeModifyMonitoredItemsRequest(
    const boost::json::value& json);
boost::json::value EncodeModifyMonitoredItemsResponse(
    const OpcUaWsModifyMonitoredItemsResponse& response);
OpcUaWsModifyMonitoredItemsResponse DecodeModifyMonitoredItemsResponse(
    const boost::json::value& json);
boost::json::value EncodeDeleteMonitoredItemsRequest(
    const OpcUaWsDeleteMonitoredItemsRequest& request);
OpcUaWsDeleteMonitoredItemsRequest DecodeDeleteMonitoredItemsRequest(
    const boost::json::value& json);
boost::json::value EncodeDeleteMonitoredItemsResponse(
    const OpcUaWsDeleteMonitoredItemsResponse& response);
OpcUaWsDeleteMonitoredItemsResponse DecodeDeleteMonitoredItemsResponse(
    const boost::json::value& json);
boost::json::value EncodeSetMonitoringModeRequest(
    const OpcUaWsSetMonitoringModeRequest& request);
OpcUaWsSetMonitoringModeRequest DecodeSetMonitoringModeRequest(
    const boost::json::value& json);
boost::json::value EncodeSetMonitoringModeResponse(
    const OpcUaWsSetMonitoringModeResponse& response);
OpcUaWsSetMonitoringModeResponse DecodeSetMonitoringModeResponse(
    const boost::json::value& json);
}  // namespace detail

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

std::int64_t RequireInt64(const value& json) {
  if (json.is_int64())
    return json.as_int64();
  if (json.is_uint64() &&
      json.as_uint64() <=
          static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
    return static_cast<std::int64_t>(json.as_uint64());
  }
  ThrowJsonError("Expected JSON integer");
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
  auto node_id = scada::NodeId::FromString(RequireString(json));
  if (node_id.is_null() && RequireString(json) != "i=0")
    ThrowJsonError("Invalid NodeId");
  return node_id;
}

value EncodeLocalizedText(const scada::LocalizedText& text) {
  return string(UtfConvert<char>(text));
}

scada::LocalizedText DecodeLocalizedText(const value& json) {
  return UtfConvert<char16_t>(std::string{RequireString(json)});
}

value EncodeByteString(const scada::ByteString& bytes) {
  array result;
  result.reserve(bytes.size());
  for (char byte : bytes)
    result.emplace_back(static_cast<std::uint64_t>(
        static_cast<unsigned char>(byte)));
  return result;
}

scada::ByteString DecodeByteString(const value& json) {
  scada::ByteString bytes;
  for (const auto& entry : RequireArray(json)) {
    const auto raw = RequireUInt64(entry);
    if (raw > std::numeric_limits<unsigned char>::max())
      ThrowJsonError("ByteString element out of range");
    bytes.push_back(
        static_cast<char>(static_cast<unsigned char>(raw)));
  }
  return bytes;
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
  for (const auto& value : values)
    result.emplace_back(encoder(value));
  return result;
}

template <class T, class Decoder>
std::vector<T> DecodeList(const value& json, Decoder&& decoder) {
  std::vector<T> result;
  for (const auto& entry : RequireArray(json))
    result.emplace_back(decoder(entry));
  return result;
}

value EncodeCreateSessionRequest(const OpcUaWsCreateSessionRequest& request) {
  return object{
      {"requestedTimeoutMs", request.requested_timeout.InMilliseconds()}};
}

OpcUaWsCreateSessionRequest DecodeCreateSessionRequest(const value& json) {
  return {.requested_timeout = base::TimeDelta::FromMilliseconds(
              RequireInt64(
                  RequireField(RequireObject(json), "requestedTimeoutMs")))};
}

value EncodeCreateSessionResponse(const OpcUaWsCreateSessionResponse& response) {
  return object{{"status", EncodeStatus(response.status)},
                {"sessionId", EncodeNodeId(response.session_id)},
                {"authenticationToken",
                 EncodeNodeId(response.authentication_token)},
                {"serverNonce", EncodeByteString(response.server_nonce)},
                {"revisedTimeoutMs",
                 response.revised_timeout.InMilliseconds()}};
}

OpcUaWsCreateSessionResponse DecodeCreateSessionResponse(const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "status")),
          .session_id = DecodeNodeId(RequireField(obj, "sessionId")),
          .authentication_token =
              DecodeNodeId(RequireField(obj, "authenticationToken")),
          .server_nonce = DecodeByteString(RequireField(obj, "serverNonce")),
          .revised_timeout = base::TimeDelta::FromMilliseconds(
              RequireInt64(RequireField(obj, "revisedTimeoutMs")))};
}

value EncodeActivateSessionRequest(const OpcUaWsActivateSessionRequest& request) {
  object json{{"sessionId", EncodeNodeId(request.session_id)},
              {"authenticationToken",
               EncodeNodeId(request.authentication_token)},
              {"deleteExisting", request.delete_existing},
              {"allowAnonymous", request.allow_anonymous}};
  if (request.user_name.has_value())
    json["userName"] = EncodeLocalizedText(*request.user_name);
  if (request.password.has_value())
    json["password"] = EncodeLocalizedText(*request.password);
  return json;
}

OpcUaWsActivateSessionRequest DecodeActivateSessionRequest(const value& json) {
  const auto& obj = RequireObject(json);
  OpcUaWsActivateSessionRequest request{
      .session_id = DecodeNodeId(RequireField(obj, "sessionId")),
      .authentication_token =
          DecodeNodeId(RequireField(obj, "authenticationToken")),
      .delete_existing = RequireBool(RequireField(obj, "deleteExisting")),
      .allow_anonymous = RequireBool(RequireField(obj, "allowAnonymous")),
  };
  if (const auto* field = FindField(obj, "userName"))
    request.user_name = DecodeLocalizedText(*field);
  if (const auto* field = FindField(obj, "password"))
    request.password = DecodeLocalizedText(*field);
  return request;
}

value EncodeActivateSessionResponse(
    const OpcUaWsActivateSessionResponse& response) {
  return object{{"status", EncodeStatus(response.status)},
                {"resumed", response.resumed}};
}

OpcUaWsActivateSessionResponse DecodeActivateSessionResponse(const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "status")),
          .resumed = RequireBool(RequireField(obj, "resumed"))};
}

value EncodeCloseSessionRequest(const OpcUaWsCloseSessionRequest& request) {
  return object{{"sessionId", EncodeNodeId(request.session_id)},
                {"authenticationToken",
                 EncodeNodeId(request.authentication_token)}};
}

OpcUaWsCloseSessionRequest DecodeCloseSessionRequest(const value& json) {
  const auto& obj = RequireObject(json);
  return {.session_id = DecodeNodeId(RequireField(obj, "sessionId")),
          .authentication_token =
              DecodeNodeId(RequireField(obj, "authenticationToken"))};
}

value EncodeCloseSessionResponse(const OpcUaWsCloseSessionResponse& response) {
  return object{{"status", EncodeStatus(response.status)}};
}

OpcUaWsCloseSessionResponse DecodeCloseSessionResponse(const value& json) {
  return {.status = DecodeStatus(RequireField(RequireObject(json), "status"))};
}

value EncodeServiceFault(const OpcUaWsServiceFault& fault) {
  return object{{"status", EncodeStatus(fault.status)}};
}

OpcUaWsServiceFault DecodeServiceFault(const value& json) {
  return {.status = DecodeStatus(RequireField(RequireObject(json), "status"))};
}

}  // namespace

boost::json::value EncodeJson(const OpcUaWsRequestMessage& request) {
  return std::visit(
      [&](const auto& typed_request) -> value {
        object json;
        json["requestHandle"] = request.request_handle;
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, OpcUaWsCreateSessionRequest>) {
          json["service"] = "CreateSession";
          json["body"] = EncodeCreateSessionRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsActivateSessionRequest>) {
          json["service"] = "ActivateSession";
          json["body"] = EncodeActivateSessionRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsCloseSessionRequest>) {
          json["service"] = "CloseSession";
          json["body"] = EncodeCloseSessionRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsCreateSubscriptionRequest>) {
          json["service"] = "CreateSubscription";
          json["body"] = detail::EncodeCreateSubscriptionRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsModifySubscriptionRequest>) {
          json["service"] = "ModifySubscription";
          json["body"] = detail::EncodeModifySubscriptionRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsSetPublishingModeRequest>) {
          json["service"] = "SetPublishingMode";
          json["body"] = detail::EncodeSetPublishingModeRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsDeleteSubscriptionsRequest>) {
          json["service"] = "DeleteSubscriptions";
          json["body"] = detail::EncodeDeleteSubscriptionsRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsCreateMonitoredItemsRequest>) {
          json["service"] = "CreateMonitoredItems";
          json["body"] = detail::EncodeCreateMonitoredItemsRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsModifyMonitoredItemsRequest>) {
          json["service"] = "ModifyMonitoredItems";
          json["body"] = detail::EncodeModifyMonitoredItemsRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsDeleteMonitoredItemsRequest>) {
          json["service"] = "DeleteMonitoredItems";
          json["body"] = detail::EncodeDeleteMonitoredItemsRequest(typed_request);
        } else if constexpr (std::is_same_v<T, OpcUaWsSetMonitoringModeRequest>) {
          json["service"] = "SetMonitoringMode";
          json["body"] = detail::EncodeSetMonitoringModeRequest(typed_request);
        } else {
          const auto service_json =
              RequireObject(EncodeJson(OpcUaWsServiceRequest{typed_request}));
          json["service"] = service_json.at("service");
          json["body"] = service_json.at("body");
        }
        return json;
      },
      request.body);
}

boost::json::value EncodeJson(const OpcUaWsResponseMessage& response) {
  return std::visit(
      [&](const auto& typed_response) -> value {
        object json;
        json["requestHandle"] = response.request_handle;
        using T = std::decay_t<decltype(typed_response)>;
        if constexpr (std::is_same_v<T, OpcUaWsCreateSessionResponse>) {
          json["service"] = "CreateSession";
          json["body"] = EncodeCreateSessionResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsActivateSessionResponse>) {
          json["service"] = "ActivateSession";
          json["body"] = EncodeActivateSessionResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsCloseSessionResponse>) {
          json["service"] = "CloseSession";
          json["body"] = EncodeCloseSessionResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsCreateSubscriptionResponse>) {
          json["service"] = "CreateSubscription";
          json["body"] = detail::EncodeCreateSubscriptionResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsModifySubscriptionResponse>) {
          json["service"] = "ModifySubscription";
          json["body"] = detail::EncodeModifySubscriptionResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsSetPublishingModeResponse>) {
          json["service"] = "SetPublishingMode";
          json["body"] = detail::EncodeSetPublishingModeResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsDeleteSubscriptionsResponse>) {
          json["service"] = "DeleteSubscriptions";
          json["body"] = detail::EncodeDeleteSubscriptionsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsCreateMonitoredItemsResponse>) {
          json["service"] = "CreateMonitoredItems";
          json["body"] = detail::EncodeCreateMonitoredItemsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsModifyMonitoredItemsResponse>) {
          json["service"] = "ModifyMonitoredItems";
          json["body"] = detail::EncodeModifyMonitoredItemsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsDeleteMonitoredItemsResponse>) {
          json["service"] = "DeleteMonitoredItems";
          json["body"] = detail::EncodeDeleteMonitoredItemsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsSetMonitoringModeResponse>) {
          json["service"] = "SetMonitoringMode";
          json["body"] = detail::EncodeSetMonitoringModeResponse(typed_response);
        } else if constexpr (std::is_same_v<T, OpcUaWsServiceFault>) {
          json["service"] = "ServiceFault";
          json["body"] = EncodeServiceFault(typed_response);
        } else {
          const auto service_json =
              RequireObject(EncodeJson(OpcUaWsServiceResponse{typed_response}));
          json["service"] = service_json.at("service");
          json["body"] = service_json.at("body");
        }
        return json;
      },
      response.body);
}

OpcUaWsRequestMessage DecodeRequestMessage(const boost::json::value& json) {
  const auto& obj = RequireObject(json);
  const auto& body = RequireField(obj, "body");
  const auto service = RequireString(RequireField(obj, "service"));
  OpcUaWsRequestMessage message{
      .request_handle = static_cast<scada::UInt32>(
          RequireUInt64(RequireField(obj, "requestHandle"))),
      .body = OpcUaWsCloseSessionRequest{},
  };
  if (service == "CreateSession") {
    message.body = DecodeCreateSessionRequest(body);
  } else if (service == "ActivateSession") {
    message.body = DecodeActivateSessionRequest(body);
  } else if (service == "CloseSession") {
    message.body = DecodeCloseSessionRequest(body);
  } else if (service == "CreateSubscription") {
    message.body = detail::DecodeCreateSubscriptionRequest(body);
  } else if (service == "ModifySubscription") {
    message.body = detail::DecodeModifySubscriptionRequest(body);
  } else if (service == "SetPublishingMode") {
    message.body = detail::DecodeSetPublishingModeRequest(body);
  } else if (service == "DeleteSubscriptions") {
    message.body = detail::DecodeDeleteSubscriptionsRequest(body);
  } else if (service == "CreateMonitoredItems") {
    message.body = detail::DecodeCreateMonitoredItemsRequest(body);
  } else if (service == "ModifyMonitoredItems") {
    message.body = detail::DecodeModifyMonitoredItemsRequest(body);
  } else if (service == "DeleteMonitoredItems") {
    message.body = detail::DecodeDeleteMonitoredItemsRequest(body);
  } else if (service == "SetMonitoringMode") {
    message.body = detail::DecodeSetMonitoringModeRequest(body);
  } else {
    message.body = std::visit(
        [](auto&& typed_request) -> OpcUaWsRequestBody {
          return std::forward<decltype(typed_request)>(typed_request);
        },
        DecodeServiceRequest(json));
  }
  return message;
}

OpcUaWsResponseMessage DecodeResponseMessage(const boost::json::value& json) {
  const auto& obj = RequireObject(json);
  const auto& body = RequireField(obj, "body");
  const auto service = RequireString(RequireField(obj, "service"));
  OpcUaWsResponseMessage message{
      .request_handle = static_cast<scada::UInt32>(
          RequireUInt64(RequireField(obj, "requestHandle"))),
      .body = OpcUaWsCloseSessionResponse{},
  };
  if (service == "CreateSession") {
    message.body = DecodeCreateSessionResponse(body);
  } else if (service == "ActivateSession") {
    message.body = DecodeActivateSessionResponse(body);
  } else if (service == "CloseSession") {
    message.body = DecodeCloseSessionResponse(body);
  } else if (service == "CreateSubscription") {
    message.body = detail::DecodeCreateSubscriptionResponse(body);
  } else if (service == "ModifySubscription") {
    message.body = detail::DecodeModifySubscriptionResponse(body);
  } else if (service == "SetPublishingMode") {
    message.body = detail::DecodeSetPublishingModeResponse(body);
  } else if (service == "DeleteSubscriptions") {
    message.body = detail::DecodeDeleteSubscriptionsResponse(body);
  } else if (service == "CreateMonitoredItems") {
    message.body = detail::DecodeCreateMonitoredItemsResponse(body);
  } else if (service == "ModifyMonitoredItems") {
    message.body = detail::DecodeModifyMonitoredItemsResponse(body);
  } else if (service == "DeleteMonitoredItems") {
    message.body = detail::DecodeDeleteMonitoredItemsResponse(body);
  } else if (service == "SetMonitoringMode") {
    message.body = detail::DecodeSetMonitoringModeResponse(body);
  } else if (service == "ServiceFault") {
    message.body = DecodeServiceFault(body);
  } else {
    message.body = std::visit(
        [](auto&& typed_response) -> OpcUaWsResponseBody {
          return std::forward<decltype(typed_response)>(typed_response);
        },
        DecodeServiceResponse(json));
  }
  return message;
}

}  // namespace opcua_ws
