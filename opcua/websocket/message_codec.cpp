#include "opcua/websocket/json_codec.h"

#include "base/utf_convert.h"

#include <boost/json.hpp>

#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace opcua {

namespace detail {
boost::json::value EncodeCreateSubscriptionRequest(
    const opcua::OpcUaCreateSubscriptionRequest& request);
opcua::OpcUaCreateSubscriptionRequest DecodeCreateSubscriptionRequest(
    const boost::json::value& json);
boost::json::value EncodeCreateSubscriptionResponse(
    const opcua::OpcUaCreateSubscriptionResponse& response);
opcua::OpcUaCreateSubscriptionResponse DecodeCreateSubscriptionResponse(
    const boost::json::value& json);
boost::json::value EncodeModifySubscriptionRequest(
    const opcua::OpcUaModifySubscriptionRequest& request);
opcua::OpcUaModifySubscriptionRequest DecodeModifySubscriptionRequest(
    const boost::json::value& json);
boost::json::value EncodeModifySubscriptionResponse(
    const opcua::OpcUaModifySubscriptionResponse& response);
opcua::OpcUaModifySubscriptionResponse DecodeModifySubscriptionResponse(
    const boost::json::value& json);
boost::json::value EncodeSetPublishingModeRequest(
    const opcua::OpcUaSetPublishingModeRequest& request);
opcua::OpcUaSetPublishingModeRequest DecodeSetPublishingModeRequest(
    const boost::json::value& json);
boost::json::value EncodeSetPublishingModeResponse(
    const opcua::OpcUaSetPublishingModeResponse& response);
opcua::OpcUaSetPublishingModeResponse DecodeSetPublishingModeResponse(
    const boost::json::value& json);
boost::json::value EncodeDeleteSubscriptionsRequest(
    const opcua::OpcUaDeleteSubscriptionsRequest& request);
opcua::OpcUaDeleteSubscriptionsRequest DecodeDeleteSubscriptionsRequest(
    const boost::json::value& json);
boost::json::value EncodeDeleteSubscriptionsResponse(
    const opcua::OpcUaDeleteSubscriptionsResponse& response);
opcua::OpcUaDeleteSubscriptionsResponse DecodeDeleteSubscriptionsResponse(
    const boost::json::value& json);
boost::json::value EncodePublishRequest(const opcua::OpcUaPublishRequest& request);
opcua::OpcUaPublishRequest DecodePublishRequest(const boost::json::value& json);
boost::json::value EncodePublishResponse(
    const opcua::OpcUaPublishResponse& response);
opcua::OpcUaPublishResponse DecodePublishResponse(
    const boost::json::value& json);
boost::json::value EncodeRepublishRequest(
    const opcua::OpcUaRepublishRequest& request);
opcua::OpcUaRepublishRequest DecodeRepublishRequest(
    const boost::json::value& json);
boost::json::value EncodeRepublishResponse(
    const opcua::OpcUaRepublishResponse& response);
opcua::OpcUaRepublishResponse DecodeRepublishResponse(
    const boost::json::value& json);
boost::json::value EncodeTransferSubscriptionsRequest(
    const opcua::OpcUaTransferSubscriptionsRequest& request);
opcua::OpcUaTransferSubscriptionsRequest DecodeTransferSubscriptionsRequest(
    const boost::json::value& json);
boost::json::value EncodeTransferSubscriptionsResponse(
    const opcua::OpcUaTransferSubscriptionsResponse& response);
opcua::OpcUaTransferSubscriptionsResponse DecodeTransferSubscriptionsResponse(
    const boost::json::value& json);
boost::json::value EncodeCreateMonitoredItemsRequest(
    const opcua::OpcUaCreateMonitoredItemsRequest& request);
opcua::OpcUaCreateMonitoredItemsRequest DecodeCreateMonitoredItemsRequest(
    const boost::json::value& json);
boost::json::value EncodeCreateMonitoredItemsResponse(
    const opcua::OpcUaCreateMonitoredItemsResponse& response);
opcua::OpcUaCreateMonitoredItemsResponse DecodeCreateMonitoredItemsResponse(
    const boost::json::value& json);
boost::json::value EncodeModifyMonitoredItemsRequest(
    const opcua::OpcUaModifyMonitoredItemsRequest& request);
opcua::OpcUaModifyMonitoredItemsRequest DecodeModifyMonitoredItemsRequest(
    const boost::json::value& json);
boost::json::value EncodeModifyMonitoredItemsResponse(
    const opcua::OpcUaModifyMonitoredItemsResponse& response);
opcua::OpcUaModifyMonitoredItemsResponse DecodeModifyMonitoredItemsResponse(
    const boost::json::value& json);
boost::json::value EncodeDeleteMonitoredItemsRequest(
    const opcua::OpcUaDeleteMonitoredItemsRequest& request);
opcua::OpcUaDeleteMonitoredItemsRequest DecodeDeleteMonitoredItemsRequest(
    const boost::json::value& json);
boost::json::value EncodeDeleteMonitoredItemsResponse(
    const opcua::OpcUaDeleteMonitoredItemsResponse& response);
opcua::OpcUaDeleteMonitoredItemsResponse DecodeDeleteMonitoredItemsResponse(
    const boost::json::value& json);
boost::json::value EncodeSetMonitoringModeRequest(
    const opcua::OpcUaSetMonitoringModeRequest& request);
opcua::OpcUaSetMonitoringModeRequest DecodeSetMonitoringModeRequest(
    const boost::json::value& json);
boost::json::value EncodeSetMonitoringModeResponse(
    const opcua::OpcUaSetMonitoringModeResponse& response);
opcua::OpcUaSetMonitoringModeResponse DecodeSetMonitoringModeResponse(
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

// OPC UA Part 6 §5.4.2.14: LocalizedText is an object `{ Locale?, Text? }`,
// each field omitted when null/empty. The scada LocalizedText carries text
// only, so we only ever emit `Text`.
value EncodeLocalizedText(const scada::LocalizedText& text) {
  object json;
  std::string utf8 = UtfConvert<char>(text);
  if (!utf8.empty())
    json["Text"] = std::move(utf8);
  return json;
}

scada::LocalizedText DecodeLocalizedText(const value& json) {
  const auto& obj = RequireObject(json);
  if (const auto* text = FindField(obj, "Text"))
    return UtfConvert<char16_t>(std::string{RequireString(*text)});
  return {};
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

value EncodeCreateSessionRequest(const opcua::OpcUaCreateSessionRequest& request) {
  return object{
      {"RequestedSessionTimeout", request.requested_timeout.InMilliseconds()}};
}

opcua::OpcUaCreateSessionRequest DecodeCreateSessionRequest(const value& json) {
  return {.requested_timeout = base::TimeDelta::FromMilliseconds(
              RequireInt64(
                  RequireField(RequireObject(json), "RequestedSessionTimeout")))};
}

value EncodeCreateSessionResponse(
    const opcua::OpcUaCreateSessionResponse& response) {
  return object{{"Status", EncodeStatus(response.status)},
                {"SessionId", EncodeNodeId(response.session_id)},
                {"AuthenticationToken",
                 EncodeNodeId(response.authentication_token)},
                {"ServerNonce", EncodeByteString(response.server_nonce)},
                {"RevisedSessionTimeout",
                 response.revised_timeout.InMilliseconds()}};
}

opcua::OpcUaCreateSessionResponse DecodeCreateSessionResponse(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "Status")),
          .session_id = DecodeNodeId(RequireField(obj, "SessionId")),
          .authentication_token =
              DecodeNodeId(RequireField(obj, "AuthenticationToken")),
          .server_nonce = DecodeByteString(RequireField(obj, "ServerNonce")),
          .revised_timeout = base::TimeDelta::FromMilliseconds(
              RequireInt64(RequireField(obj, "RevisedSessionTimeout")))};
}

value EncodeActivateSessionRequest(
    const opcua::OpcUaActivateSessionRequest& request) {
  object json{{"SessionId", EncodeNodeId(request.session_id)},
              {"AuthenticationToken",
               EncodeNodeId(request.authentication_token)},
              {"DeleteExisting", request.delete_existing},
              {"AllowAnonymous", request.allow_anonymous}};
  // UserName / Password are plain strings on the wire — they map onto the
  // UserNameIdentityToken (Part 4 §7.36.4) which types both as String /
  // ByteString, not LocalizedText. The internal storage stays as
  // LocalizedText for now to avoid touching the session manager surface.
  if (request.user_name.has_value())
    json["UserName"] = string(UtfConvert<char>(*request.user_name));
  if (request.password.has_value())
    json["Password"] = string(UtfConvert<char>(*request.password));
  return json;
}

opcua::OpcUaActivateSessionRequest DecodeActivateSessionRequest(
    const value& json) {
  const auto& obj = RequireObject(json);
  opcua::OpcUaActivateSessionRequest request{
      .session_id = DecodeNodeId(RequireField(obj, "SessionId")),
      .authentication_token =
          DecodeNodeId(RequireField(obj, "AuthenticationToken")),
      .delete_existing = RequireBool(RequireField(obj, "DeleteExisting")),
      .allow_anonymous = RequireBool(RequireField(obj, "AllowAnonymous")),
  };
  if (const auto* field = FindField(obj, "UserName"))
    request.user_name =
        UtfConvert<char16_t>(std::string{RequireString(*field)});
  if (const auto* field = FindField(obj, "Password"))
    request.password =
        UtfConvert<char16_t>(std::string{RequireString(*field)});
  return request;
}

value EncodeActivateSessionResponse(
    const opcua::OpcUaActivateSessionResponse& response) {
  return object{{"Status", EncodeStatus(response.status)},
                {"Resumed", response.resumed}};
}

opcua::OpcUaActivateSessionResponse DecodeActivateSessionResponse(
    const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "Status")),
          .resumed = RequireBool(RequireField(obj, "Resumed"))};
}

value EncodeCloseSessionRequest(const opcua::OpcUaCloseSessionRequest& request) {
  return object{{"SessionId", EncodeNodeId(request.session_id)},
                {"AuthenticationToken",
                 EncodeNodeId(request.authentication_token)}};
}

opcua::OpcUaCloseSessionRequest DecodeCloseSessionRequest(const value& json) {
  const auto& obj = RequireObject(json);
  return {.session_id = DecodeNodeId(RequireField(obj, "SessionId")),
          .authentication_token =
              DecodeNodeId(RequireField(obj, "AuthenticationToken"))};
}

value EncodeCloseSessionResponse(
    const opcua::OpcUaCloseSessionResponse& response) {
  return object{{"Status", EncodeStatus(response.status)}};
}

opcua::OpcUaCloseSessionResponse DecodeCloseSessionResponse(const value& json) {
  return {.status = DecodeStatus(RequireField(RequireObject(json), "Status"))};
}

value EncodeServiceFault(const opcua::OpcUaServiceFault& fault) {
  return object{{"Status", EncodeStatus(fault.status)}};
}

opcua::OpcUaServiceFault DecodeServiceFault(const value& json) {
  return {.status = DecodeStatus(RequireField(RequireObject(json), "Status"))};
}

}  // namespace

boost::json::value EncodeJson(const opcua::OpcUaRequestMessage& request) {
  return std::visit(
      [&](const auto& typed_request) -> value {
        object json;
        json["requestHandle"] = request.request_handle;
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, opcua::OpcUaCreateSessionRequest>) {
          json["service"] = "CreateSession";
          json["body"] = EncodeCreateSessionRequest(typed_request);
        } else if constexpr (std::is_same_v<T,
                                            opcua::OpcUaActivateSessionRequest>) {
          json["service"] = "ActivateSession";
          json["body"] = EncodeActivateSessionRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaCloseSessionRequest>) {
          json["service"] = "CloseSession";
          json["body"] = EncodeCloseSessionRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaCreateSubscriptionRequest>) {
          json["service"] = "CreateSubscription";
          json["body"] = detail::EncodeCreateSubscriptionRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaModifySubscriptionRequest>) {
          json["service"] = "ModifySubscription";
          json["body"] = detail::EncodeModifySubscriptionRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaSetPublishingModeRequest>) {
          json["service"] = "SetPublishingMode";
          json["body"] = detail::EncodeSetPublishingModeRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaDeleteSubscriptionsRequest>) {
          json["service"] = "DeleteSubscriptions";
          json["body"] = detail::EncodeDeleteSubscriptionsRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaPublishRequest>) {
          json["service"] = "Publish";
          json["body"] = detail::EncodePublishRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaRepublishRequest>) {
          json["service"] = "Republish";
          json["body"] = detail::EncodeRepublishRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaTransferSubscriptionsRequest>) {
          json["service"] = "TransferSubscriptions";
          json["body"] = detail::EncodeTransferSubscriptionsRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaCreateMonitoredItemsRequest>) {
          json["service"] = "CreateMonitoredItems";
          json["body"] = detail::EncodeCreateMonitoredItemsRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaModifyMonitoredItemsRequest>) {
          json["service"] = "ModifyMonitoredItems";
          json["body"] = detail::EncodeModifyMonitoredItemsRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaDeleteMonitoredItemsRequest>) {
          json["service"] = "DeleteMonitoredItems";
          json["body"] = detail::EncodeDeleteMonitoredItemsRequest(typed_request);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaSetMonitoringModeRequest>) {
          json["service"] = "SetMonitoringMode";
          json["body"] = detail::EncodeSetMonitoringModeRequest(typed_request);
        } else {
          const auto service_json =
              RequireObject(EncodeJson(opcua::OpcUaServiceRequest{typed_request}));
          json["service"] = service_json.at("service");
          json["body"] = service_json.at("body");
        }
        return json;
      },
      request.body);
}

boost::json::value EncodeJson(const opcua::OpcUaResponseMessage& response) {
  return std::visit(
      [&](const auto& typed_response) -> value {
        object json;
        json["requestHandle"] = response.request_handle;
        using T = std::decay_t<decltype(typed_response)>;
        if constexpr (std::is_same_v<T, opcua::OpcUaCreateSessionResponse>) {
          json["service"] = "CreateSession";
          json["body"] = EncodeCreateSessionResponse(typed_response);
        } else if constexpr (std::is_same_v<T,
                                            opcua::OpcUaActivateSessionResponse>) {
          json["service"] = "ActivateSession";
          json["body"] = EncodeActivateSessionResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaCloseSessionResponse>) {
          json["service"] = "CloseSession";
          json["body"] = EncodeCloseSessionResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaCreateSubscriptionResponse>) {
          json["service"] = "CreateSubscription";
          json["body"] = detail::EncodeCreateSubscriptionResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaModifySubscriptionResponse>) {
          json["service"] = "ModifySubscription";
          json["body"] = detail::EncodeModifySubscriptionResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaSetPublishingModeResponse>) {
          json["service"] = "SetPublishingMode";
          json["body"] = detail::EncodeSetPublishingModeResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaDeleteSubscriptionsResponse>) {
          json["service"] = "DeleteSubscriptions";
          json["body"] = detail::EncodeDeleteSubscriptionsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaPublishResponse>) {
          json["service"] = "Publish";
          json["body"] = detail::EncodePublishResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaRepublishResponse>) {
          json["service"] = "Republish";
          json["body"] = detail::EncodeRepublishResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaTransferSubscriptionsResponse>) {
          json["service"] = "TransferSubscriptions";
          json["body"] = detail::EncodeTransferSubscriptionsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaCreateMonitoredItemsResponse>) {
          json["service"] = "CreateMonitoredItems";
          json["body"] = detail::EncodeCreateMonitoredItemsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaModifyMonitoredItemsResponse>) {
          json["service"] = "ModifyMonitoredItems";
          json["body"] = detail::EncodeModifyMonitoredItemsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaDeleteMonitoredItemsResponse>) {
          json["service"] = "DeleteMonitoredItems";
          json["body"] = detail::EncodeDeleteMonitoredItemsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaSetMonitoringModeResponse>) {
          json["service"] = "SetMonitoringMode";
          json["body"] = detail::EncodeSetMonitoringModeResponse(typed_response);
        } else if constexpr (std::is_same_v<T, opcua::OpcUaServiceFault>) {
          json["service"] = "ServiceFault";
          json["body"] = EncodeServiceFault(typed_response);
        } else {
          const auto service_json =
              RequireObject(EncodeJson(opcua::OpcUaServiceResponse{typed_response}));
          json["service"] = service_json.at("service");
          json["body"] = service_json.at("body");
        }
        return json;
      },
      response.body);
}

opcua::OpcUaRequestMessage DecodeRequestMessage(const boost::json::value& json) {
  const auto& obj = RequireObject(json);
  const auto& body = RequireField(obj, "body");
  const auto service = RequireString(RequireField(obj, "service"));
  opcua::OpcUaRequestMessage message{
      .request_handle = static_cast<scada::UInt32>(
          RequireUInt64(RequireField(obj, "requestHandle"))),
      .body = opcua::OpcUaCloseSessionRequest{},
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
  } else if (service == "Publish") {
    message.body = detail::DecodePublishRequest(body);
  } else if (service == "Republish") {
    message.body = detail::DecodeRepublishRequest(body);
  } else if (service == "TransferSubscriptions") {
    message.body = detail::DecodeTransferSubscriptionsRequest(body);
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
        [](auto&& typed_request) -> opcua::OpcUaRequestBody {
          return std::forward<decltype(typed_request)>(typed_request);
        },
        DecodeServiceRequest(json));
  }
  return message;
}

opcua::OpcUaResponseMessage DecodeResponseMessage(const boost::json::value& json) {
  const auto& obj = RequireObject(json);
  const auto& body = RequireField(obj, "body");
  const auto service = RequireString(RequireField(obj, "service"));
  opcua::OpcUaResponseMessage message{
      .request_handle = static_cast<scada::UInt32>(
          RequireUInt64(RequireField(obj, "requestHandle"))),
      .body = opcua::OpcUaCloseSessionResponse{},
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
  } else if (service == "Publish") {
    message.body = detail::DecodePublishResponse(body);
  } else if (service == "Republish") {
    message.body = detail::DecodeRepublishResponse(body);
  } else if (service == "TransferSubscriptions") {
    message.body = detail::DecodeTransferSubscriptionsResponse(body);
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
        [](auto&& typed_response) -> opcua::OpcUaResponseBody {
          return std::forward<decltype(typed_response)>(typed_response);
        },
        DecodeServiceResponse(json));
  }
  return message;
}

}  // namespace opcua
