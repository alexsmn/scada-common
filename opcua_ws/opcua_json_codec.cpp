#include "opcua_ws/opcua_json_codec.h"

#include "base/time_utils.h"
#include "base/utf_convert.h"

#include <boost/json.hpp>

#include <limits>
#include <stdexcept>
#include <string_view>

namespace opcua_ws {

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

const value& RequireField(const object& json, std::string_view key) {
  if (const auto* field = json.if_contains(key))
    return *field;
  ThrowJsonError("Missing required field");
}

const value* FindField(const object& json, std::string_view key) {
  return json.if_contains(key);
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

value EncodeExpandedNodeId(const scada::ExpandedNodeId& node_id) {
  object json;
  json["nodeId"] = EncodeNodeId(node_id.node_id());
  if (!node_id.namespace_uri().empty())
    json["namespaceUri"] = node_id.namespace_uri();
  if (node_id.server_index() != 0)
    json["serverIndex"] = node_id.server_index();
  return json;
}

scada::ExpandedNodeId DecodeExpandedNodeId(const value& json) {
  const auto& obj = RequireObject(json);
  auto node_id = DecodeNodeId(RequireField(obj, "nodeId"));
  std::string namespace_uri;
  if (const auto* field = FindField(obj, "namespaceUri"))
    namespace_uri = std::string{RequireString(*field)};
  unsigned server_index = 0;
  if (const auto* field = FindField(obj, "serverIndex"))
    server_index = static_cast<unsigned>(RequireUInt64(*field));
  return {std::move(node_id), std::move(namespace_uri), server_index};
}

value EncodeQualifiedName(const scada::QualifiedName& name) {
  return object{{"namespaceIndex", name.namespace_index()}, {"name", name.name()}};
}

scada::QualifiedName DecodeQualifiedName(const value& json) {
  const auto& obj = RequireObject(json);
  return {std::string{RequireString(RequireField(obj, "name"))},
          static_cast<scada::NamespaceIndex>(
              RequireUInt64(RequireField(obj, "namespaceIndex")))};
}

value EncodeLocalizedText(const scada::LocalizedText& text) {
  return string(UtfConvert<char>(text));
}

scada::LocalizedText DecodeLocalizedText(const value& json) {
  return UtfConvert<char16_t>(std::string{RequireString(json)});
}

value EncodeDateTime(scada::DateTime time) {
  if (time.is_null())
    return nullptr;
  return string(SerializeToString(time));
}

scada::DateTime DecodeDateTime(const value& json) {
  if (json.is_null())
    return {};
  scada::DateTime time;
  if (!Deserialize(RequireString(json), time))
    ThrowJsonError("Invalid date-time");
  return time;
}

value EncodeByteString(const scada::ByteString& bytes) {
  array result;
  result.reserve(bytes.size());
  for (char byte : bytes)
    result.emplace_back(static_cast<std::uint64_t>(static_cast<unsigned char>(byte)));
  return result;
}

scada::ByteString DecodeByteString(const value& json) {
  scada::ByteString bytes;
  for (const auto& entry : RequireArray(json)) {
    auto raw = RequireUInt64(entry);
    if (raw > std::numeric_limits<unsigned char>::max())
      ThrowJsonError("ByteString element out of range");
    bytes.push_back(static_cast<char>(static_cast<unsigned char>(raw)));
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

value EncodeVariant(const scada::Variant& variant);
scada::Variant DecodeVariant(const value& json);

value EncodeDataValue(const scada::DataValue& data_value) {
  object json{
      {"value", EncodeVariant(data_value.value)},
      {"qualifier", data_value.qualifier.raw()},
      {"sourceTimestamp", EncodeDateTime(data_value.source_timestamp)},
      {"serverTimestamp", EncodeDateTime(data_value.server_timestamp)},
      {"statusCode", EncodeStatusCode(data_value.status_code)},
  };
  return json;
}

scada::DataValue DecodeDataValue(const value& json) {
  const auto& obj = RequireObject(json);
  scada::DataValue result;
  result.value = DecodeVariant(RequireField(obj, "value"));
  result.qualifier = scada::Qualifier{
      static_cast<unsigned>(RequireUInt64(RequireField(obj, "qualifier")))};
  result.source_timestamp = DecodeDateTime(RequireField(obj, "sourceTimestamp"));
  result.server_timestamp = DecodeDateTime(RequireField(obj, "serverTimestamp"));
  result.status_code = DecodeStatusCode(RequireField(obj, "statusCode"));
  return result;
}

value EncodeEventFilter(const scada::EventFilter& filter) {
  array of_type;
  of_type.reserve(filter.of_type.size());
  for (const auto& node_id : filter.of_type)
    of_type.emplace_back(EncodeNodeId(node_id));

  array child_of;
  child_of.reserve(filter.child_of.size());
  for (const auto& node_id : filter.child_of)
    child_of.emplace_back(EncodeNodeId(node_id));

  return object{{"types", filter.types},
                {"ofType", std::move(of_type)},
                {"childOf", std::move(child_of)}};
}

scada::EventFilter DecodeEventFilter(const value& json) {
  const auto& obj = RequireObject(json);
  scada::EventFilter filter;
  filter.types = static_cast<unsigned>(RequireUInt64(RequireField(obj, "types")));
  for (const auto& entry : RequireArray(RequireField(obj, "ofType")))
    filter.of_type.push_back(DecodeNodeId(entry));
  for (const auto& entry : RequireArray(RequireField(obj, "childOf")))
    filter.child_of.push_back(DecodeNodeId(entry));
  return filter;
}

value EncodeAggregateFilter(const scada::AggregateFilter& filter) {
  return object{{"startTime", EncodeDateTime(filter.start_time)},
                {"interval", filter.interval.InMicroseconds()},
                {"aggregateType", EncodeNodeId(filter.aggregate_type)}};
}

scada::AggregateFilter DecodeAggregateFilter(const value& json) {
  const auto& obj = RequireObject(json);
  return {.start_time = DecodeDateTime(RequireField(obj, "startTime")),
          .interval = base::TimeDelta::FromMicroseconds(
              RequireInt64(RequireField(obj, "interval"))),
          .aggregate_type = DecodeNodeId(RequireField(obj, "aggregateType"))};
}

value EncodeNodeClass(scada::NodeClass node_class) {
  return static_cast<std::uint64_t>(static_cast<unsigned>(node_class));
}

scada::NodeClass DecodeNodeClass(const value& json) {
  return static_cast<scada::NodeClass>(
      static_cast<unsigned>(RequireUInt64(json)));
}

value EncodeNodeAttributes(const scada::NodeAttributes& attributes) {
  object json;
  if (!attributes.browse_name.empty())
    json["browseName"] = EncodeQualifiedName(attributes.browse_name);
  if (!attributes.display_name.empty())
    json["displayName"] = EncodeLocalizedText(attributes.display_name);
  if (!attributes.data_type.is_null())
    json["dataType"] = EncodeNodeId(attributes.data_type);
  if (attributes.value.has_value())
    json["value"] = EncodeVariant(*attributes.value);
  return json;
}

scada::NodeAttributes DecodeNodeAttributes(const value& json) {
  const auto& obj = RequireObject(json);
  scada::NodeAttributes attributes;
  if (const auto* field = FindField(obj, "browseName"))
    attributes.browse_name = DecodeQualifiedName(*field);
  if (const auto* field = FindField(obj, "displayName"))
    attributes.display_name = DecodeLocalizedText(*field);
  if (const auto* field = FindField(obj, "dataType"))
    attributes.data_type = DecodeNodeId(*field);
  if (const auto* field = FindField(obj, "value"))
    attributes.value = DecodeVariant(*field);
  return attributes;
}

value EncodeEvent(const scada::Event& event) {
  return object{
      {"eventTypeId", EncodeNodeId(event.event_type_id)},
      {"eventId", event.event_id},
      {"time", EncodeDateTime(event.time)},
      {"receiveTime", EncodeDateTime(event.receive_time)},
      {"changeMask", event.change_mask},
      {"severity", event.severity},
      {"nodeId", EncodeNodeId(event.node_id)},
      {"userId", EncodeNodeId(event.user_id)},
      {"value", EncodeVariant(event.value)},
      {"qualifier", event.qualifier.raw()},
      {"message", EncodeLocalizedText(event.message)},
      {"acked", event.acked},
      {"acknowledgedTime", EncodeDateTime(event.acknowledged_time)},
      {"acknowledgedUserId", EncodeNodeId(event.acknowledged_user_id)},
  };
}

scada::Event DecodeEvent(const value& json) {
  const auto& obj = RequireObject(json);
  scada::Event event;
  event.event_type_id = DecodeNodeId(RequireField(obj, "eventTypeId"));
  event.event_id = RequireUInt64(RequireField(obj, "eventId"));
  event.time = DecodeDateTime(RequireField(obj, "time"));
  event.receive_time = DecodeDateTime(RequireField(obj, "receiveTime"));
  event.change_mask =
      static_cast<scada::UInt32>(RequireUInt64(RequireField(obj, "changeMask")));
  event.severity =
      static_cast<scada::UInt32>(RequireUInt64(RequireField(obj, "severity")));
  event.node_id = DecodeNodeId(RequireField(obj, "nodeId"));
  event.user_id = DecodeNodeId(RequireField(obj, "userId"));
  event.value = DecodeVariant(RequireField(obj, "value"));
  event.qualifier = scada::Qualifier{
      static_cast<unsigned>(RequireUInt64(RequireField(obj, "qualifier")))};
  event.message = DecodeLocalizedText(RequireField(obj, "message"));
  event.acked = RequireBool(RequireField(obj, "acked"));
  event.acknowledged_time = DecodeDateTime(RequireField(obj, "acknowledgedTime"));
  event.acknowledged_user_id = DecodeNodeId(RequireField(obj, "acknowledgedUserId"));
  return event;
}

template <class T, class Encoder>
array EncodeList(const std::vector<T>& values, Encoder&& encoder) {
  array json;
  json.reserve(values.size());
  for (const auto& value : values)
    json.emplace_back(encoder(value));
  return json;
}

template <class T, class Decoder>
std::vector<T> DecodeList(const value& json, Decoder&& decoder) {
  std::vector<T> values;
  const auto& source = RequireArray(json);
  values.reserve(source.size());
  for (const auto& entry : source)
    values.push_back(decoder(entry));
  return values;
}

template <class T>
value EncodeScalarList(const std::vector<T>& values) {
  array json;
  json.reserve(values.size());
  for (const auto& value : values)
    json.emplace_back(value);
  return json;
}

template <class T>
std::vector<T> DecodeIntList(const value& json) {
  std::vector<T> values;
  const auto& source = RequireArray(json);
  values.reserve(source.size());
  for (const auto& entry : source)
    values.push_back(static_cast<T>(RequireInt64(entry)));
  return values;
}

template <class T>
std::vector<T> DecodeUIntList(const value& json) {
  std::vector<T> values;
  const auto& source = RequireArray(json);
  values.reserve(source.size());
  for (const auto& entry : source)
    values.push_back(static_cast<T>(RequireUInt64(entry)));
  return values;
}

value EncodeVariant(const scada::Variant& variant) {
  object json{{"type", ToString(variant.type())}, {"isArray", variant.is_array()}};

  if (variant.is_scalar()) {
    switch (variant.type()) {
      case scada::Variant::EMPTY:
        json["value"] = nullptr;
        break;
      case scada::Variant::BOOL:
        json["value"] = variant.get<bool>();
        break;
      case scada::Variant::INT8:
        json["value"] = variant.get<scada::Int8>();
        break;
      case scada::Variant::UINT8:
        json["value"] = variant.get<scada::UInt8>();
        break;
      case scada::Variant::INT16:
        json["value"] = variant.get<scada::Int16>();
        break;
      case scada::Variant::UINT16:
        json["value"] = variant.get<scada::UInt16>();
        break;
      case scada::Variant::INT32:
        json["value"] = variant.get<scada::Int32>();
        break;
      case scada::Variant::UINT32:
        json["value"] = variant.get<scada::UInt32>();
        break;
      case scada::Variant::INT64:
        json["value"] = variant.get<scada::Int64>();
        break;
      case scada::Variant::UINT64:
        json["value"] = variant.get<scada::UInt64>();
        break;
      case scada::Variant::DOUBLE:
        json["value"] = variant.get<double>();
        break;
      case scada::Variant::BYTE_STRING:
        json["value"] = EncodeByteString(variant.get<scada::ByteString>());
        break;
      case scada::Variant::STRING:
        json["value"] = variant.get<scada::String>();
        break;
      case scada::Variant::QUALIFIED_NAME:
        json["value"] = EncodeQualifiedName(variant.get<scada::QualifiedName>());
        break;
      case scada::Variant::LOCALIZED_TEXT:
        json["value"] = EncodeLocalizedText(variant.get<scada::LocalizedText>());
        break;
      case scada::Variant::NODE_ID:
        json["value"] = EncodeNodeId(variant.get<scada::NodeId>());
        break;
      case scada::Variant::EXPANDED_NODE_ID:
        json["value"] =
            EncodeExpandedNodeId(variant.get<scada::ExpandedNodeId>());
        break;
      case scada::Variant::EXTENSION_OBJECT:
        ThrowJsonError("ExtensionObject codec not implemented");
      case scada::Variant::DATE_TIME:
        json["value"] = EncodeDateTime(variant.get<scada::DateTime>());
        break;
      case scada::Variant::COUNT:
        ThrowJsonError("Unexpected scalar variant type");
    }
  } else {
    switch (variant.type()) {
      case scada::Variant::EMPTY:
        json["value"] =
            array(variant.get<std::vector<std::monostate>>().size(), nullptr);
        break;
      case scada::Variant::BOOL:
        json["value"] = EncodeScalarList(variant.get<std::vector<bool>>());
        break;
      case scada::Variant::INT8:
        json["value"] = EncodeScalarList(variant.get<std::vector<scada::Int8>>());
        break;
      case scada::Variant::UINT8:
        json["value"] = EncodeScalarList(variant.get<std::vector<scada::UInt8>>());
        break;
      case scada::Variant::INT16:
        json["value"] = EncodeScalarList(variant.get<std::vector<scada::Int16>>());
        break;
      case scada::Variant::UINT16:
        json["value"] = EncodeScalarList(variant.get<std::vector<scada::UInt16>>());
        break;
      case scada::Variant::INT32:
        json["value"] = EncodeScalarList(variant.get<std::vector<scada::Int32>>());
        break;
      case scada::Variant::UINT32:
        json["value"] = EncodeScalarList(variant.get<std::vector<scada::UInt32>>());
        break;
      case scada::Variant::INT64:
        json["value"] = EncodeScalarList(variant.get<std::vector<scada::Int64>>());
        break;
      case scada::Variant::UINT64:
        json["value"] = EncodeScalarList(variant.get<std::vector<scada::UInt64>>());
        break;
      case scada::Variant::DOUBLE:
        json["value"] = EncodeScalarList(variant.get<std::vector<double>>());
        break;
      case scada::Variant::BYTE_STRING:
        json["value"] = EncodeList(variant.get<std::vector<scada::ByteString>>(),
                                   EncodeByteString);
        break;
      case scada::Variant::STRING:
        json["value"] = EncodeScalarList(variant.get<std::vector<scada::String>>());
        break;
      case scada::Variant::QUALIFIED_NAME:
        json["value"] = EncodeList(
            variant.get<std::vector<scada::QualifiedName>>(),
            EncodeQualifiedName);
        break;
      case scada::Variant::LOCALIZED_TEXT:
        json["value"] = EncodeList(
            variant.get<std::vector<scada::LocalizedText>>(),
            EncodeLocalizedText);
        break;
      case scada::Variant::NODE_ID:
        json["value"] =
            EncodeList(variant.get<std::vector<scada::NodeId>>(), EncodeNodeId);
        break;
      case scada::Variant::EXPANDED_NODE_ID:
        json["value"] = EncodeList(
            variant.get<std::vector<scada::ExpandedNodeId>>(),
            EncodeExpandedNodeId);
        break;
      case scada::Variant::DATE_TIME:
        ThrowJsonError("DateTime array codec not implemented");
      case scada::Variant::EXTENSION_OBJECT:
        ThrowJsonError("ExtensionObject array codec not implemented");
      case scada::Variant::COUNT:
        ThrowJsonError("Unexpected array variant type");
    }
  }

  return json;
}

scada::Variant DecodeVariant(const value& json) {
  const auto& obj = RequireObject(json);
  auto type = scada::ParseBuiltInType(RequireString(RequireField(obj, "type")));
  bool is_array = RequireBool(RequireField(obj, "isArray"));
  const auto& payload = RequireField(obj, "value");

  if (!is_array) {
    switch (type) {
      case scada::Variant::EMPTY:
        return {};
      case scada::Variant::BOOL:
        return scada::Variant{RequireBool(payload)};
      case scada::Variant::INT8:
        return scada::Variant{static_cast<scada::Int8>(RequireInt64(payload))};
      case scada::Variant::UINT8:
        return scada::Variant{static_cast<scada::UInt8>(RequireUInt64(payload))};
      case scada::Variant::INT16:
        return scada::Variant{static_cast<scada::Int16>(RequireInt64(payload))};
      case scada::Variant::UINT16:
        return scada::Variant{static_cast<scada::UInt16>(RequireUInt64(payload))};
      case scada::Variant::INT32:
        return scada::Variant{static_cast<scada::Int32>(RequireInt64(payload))};
      case scada::Variant::UINT32:
        return scada::Variant{static_cast<scada::UInt32>(RequireUInt64(payload))};
      case scada::Variant::INT64:
        return scada::Variant{static_cast<scada::Int64>(RequireInt64(payload))};
      case scada::Variant::UINT64:
        return scada::Variant{static_cast<scada::UInt64>(RequireUInt64(payload))};
      case scada::Variant::DOUBLE:
        return scada::Variant{RequireDouble(payload)};
      case scada::Variant::BYTE_STRING:
        return scada::Variant{DecodeByteString(payload)};
      case scada::Variant::STRING:
        return scada::Variant{std::string{RequireString(payload)}};
      case scada::Variant::QUALIFIED_NAME:
        return scada::Variant{DecodeQualifiedName(payload)};
      case scada::Variant::LOCALIZED_TEXT:
        return scada::Variant{DecodeLocalizedText(payload)};
      case scada::Variant::NODE_ID:
        return scada::Variant{DecodeNodeId(payload)};
      case scada::Variant::EXPANDED_NODE_ID:
        return scada::Variant{DecodeExpandedNodeId(payload)};
      case scada::Variant::DATE_TIME:
        return scada::Variant{DecodeDateTime(payload)};
      case scada::Variant::EXTENSION_OBJECT:
      case scada::Variant::COUNT:
        ThrowJsonError("Unsupported scalar variant type");
    }
  }

  switch (type) {
    case scada::Variant::EMPTY:
      return scada::Variant{DecodeList<std::monostate>(payload, [](const value&) {
        return std::monostate{};
      })};
    case scada::Variant::BOOL:
      return scada::Variant{DecodeList<bool>(payload, RequireBool)};
    case scada::Variant::INT8:
      return scada::Variant{DecodeIntList<scada::Int8>(payload)};
    case scada::Variant::UINT8:
      return scada::Variant{DecodeUIntList<scada::UInt8>(payload)};
    case scada::Variant::INT16:
      return scada::Variant{DecodeIntList<scada::Int16>(payload)};
    case scada::Variant::UINT16:
      return scada::Variant{DecodeUIntList<scada::UInt16>(payload)};
    case scada::Variant::INT32:
      return scada::Variant{DecodeIntList<scada::Int32>(payload)};
    case scada::Variant::UINT32:
      return scada::Variant{DecodeUIntList<scada::UInt32>(payload)};
    case scada::Variant::INT64:
      return scada::Variant{DecodeIntList<scada::Int64>(payload)};
    case scada::Variant::UINT64:
      return scada::Variant{DecodeUIntList<scada::UInt64>(payload)};
    case scada::Variant::DOUBLE:
      return scada::Variant{DecodeList<double>(payload, RequireDouble)};
    case scada::Variant::BYTE_STRING:
      return scada::Variant{
          DecodeList<scada::ByteString>(payload, DecodeByteString)};
    case scada::Variant::STRING:
      return scada::Variant{DecodeList<scada::String>(payload, [](const value& v) {
        return std::string{RequireString(v)};
      })};
    case scada::Variant::QUALIFIED_NAME:
      return scada::Variant{
          DecodeList<scada::QualifiedName>(payload, DecodeQualifiedName)};
    case scada::Variant::LOCALIZED_TEXT:
      return scada::Variant{
          DecodeList<scada::LocalizedText>(payload, DecodeLocalizedText)};
    case scada::Variant::NODE_ID:
      return scada::Variant{DecodeList<scada::NodeId>(payload, DecodeNodeId)};
    case scada::Variant::EXPANDED_NODE_ID:
      return scada::Variant{
          DecodeList<scada::ExpandedNodeId>(payload, DecodeExpandedNodeId)};
    case scada::Variant::DATE_TIME:
    case scada::Variant::EXTENSION_OBJECT:
    case scada::Variant::COUNT:
      ThrowJsonError("Unsupported array variant type");
  }

  ThrowJsonError("Unsupported variant type");
}

value EncodeHistoryReadRawRequest(const HistoryReadRawRequest& request) {
  return object{{"details",
                 object{{"nodeId", EncodeNodeId(request.details.node_id)},
                        {"from", EncodeDateTime(request.details.from)},
                        {"to", EncodeDateTime(request.details.to)},
                        {"maxCount", request.details.max_count},
                        {"aggregation",
                         EncodeAggregateFilter(request.details.aggregation)},
                        {"releaseContinuationPoint",
                         request.details.release_continuation_point},
                        {"continuationPoint",
                         EncodeByteString(request.details.continuation_point)}}}};
}

HistoryReadRawRequest DecodeHistoryReadRawRequest(const value& json) {
  const auto& details = RequireObject(RequireField(RequireObject(json), "details"));
  return {.details =
              {.node_id = DecodeNodeId(RequireField(details, "nodeId")),
               .from = DecodeDateTime(RequireField(details, "from")),
               .to = DecodeDateTime(RequireField(details, "to")),
               .max_count = static_cast<size_t>(
                   RequireUInt64(RequireField(details, "maxCount"))),
               .aggregation =
                   DecodeAggregateFilter(RequireField(details, "aggregation")),
               .release_continuation_point =
                   RequireBool(RequireField(details, "releaseContinuationPoint")),
               .continuation_point =
                   DecodeByteString(RequireField(details, "continuationPoint"))}};
}

value EncodeHistoryReadEventsRequest(const HistoryReadEventsRequest& request) {
  return object{{"details",
                 object{{"nodeId", EncodeNodeId(request.details.node_id)},
                        {"from", EncodeDateTime(request.details.from)},
                        {"to", EncodeDateTime(request.details.to)},
                        {"filter", EncodeEventFilter(request.details.filter)}}}};
}

HistoryReadEventsRequest DecodeHistoryReadEventsRequest(const value& json) {
  const auto& details = RequireObject(RequireField(RequireObject(json), "details"));
  return {.details =
              {.node_id = DecodeNodeId(RequireField(details, "nodeId")),
               .from = DecodeDateTime(RequireField(details, "from")),
               .to = DecodeDateTime(RequireField(details, "to")),
               .filter = DecodeEventFilter(RequireField(details, "filter"))}};
}

value EncodeCallRequest(const CallRequest& request) {
  return object{{"methods",
                 EncodeList(request.methods, [](const CallMethodRequest& method) {
                   return object{{"objectId", EncodeNodeId(method.object_id)},
                                 {"methodId", EncodeNodeId(method.method_id)},
                                 {"arguments", EncodeList(method.arguments, EncodeVariant)}};
                 })}};
}

CallRequest DecodeCallRequest(const value& json) {
  return {.methods = DecodeList<CallMethodRequest>(
              RequireField(RequireObject(json), "methods"),
              [](const value& entry) {
                const auto& obj = RequireObject(entry);
                return CallMethodRequest{
                    .object_id = DecodeNodeId(RequireField(obj, "objectId")),
                    .method_id = DecodeNodeId(RequireField(obj, "methodId")),
                    .arguments = DecodeList<scada::Variant>(
                        RequireField(obj, "arguments"), DecodeVariant)};
              })};
}

value EncodeAddNodesRequest(const AddNodesRequest& request) {
  return object{{"items",
                 EncodeList(request.items, [](const scada::AddNodesItem& item) {
                   return object{{"requestedId", EncodeNodeId(item.requested_id)},
                                 {"parentId", EncodeNodeId(item.parent_id)},
                                 {"nodeClass", EncodeNodeClass(item.node_class)},
                                 {"typeDefinitionId",
                                  EncodeNodeId(item.type_definition_id)},
                                 {"attributes", EncodeNodeAttributes(item.attributes)}};
                 })}};
}

AddNodesRequest DecodeAddNodesRequest(const value& json) {
  return {.items = DecodeList<scada::AddNodesItem>(
              RequireField(RequireObject(json), "items"), [](const value& entry) {
                const auto& obj = RequireObject(entry);
                return scada::AddNodesItem{
                    .requested_id = DecodeNodeId(RequireField(obj, "requestedId")),
                    .parent_id = DecodeNodeId(RequireField(obj, "parentId")),
                    .node_class = DecodeNodeClass(RequireField(obj, "nodeClass")),
                    .type_definition_id =
                        DecodeNodeId(RequireField(obj, "typeDefinitionId")),
                    .attributes =
                        DecodeNodeAttributes(RequireField(obj, "attributes"))};
              })};
}

value EncodeDeleteNodesRequest(const DeleteNodesRequest& request) {
  return object{{"items",
                 EncodeList(request.items,
                            [](const scada::DeleteNodesItem& item) {
                              return object{
                                  {"nodeId", EncodeNodeId(item.node_id)},
                                  {"deleteTargetReferences",
                                   item.delete_target_references}};
                            })}};
}

DeleteNodesRequest DecodeDeleteNodesRequest(const value& json) {
  return {.items = DecodeList<scada::DeleteNodesItem>(
              RequireField(RequireObject(json), "items"), [](const value& entry) {
                const auto& obj = RequireObject(entry);
                return scada::DeleteNodesItem{
                    .node_id = DecodeNodeId(RequireField(obj, "nodeId")),
                    .delete_target_references =
                        RequireBool(RequireField(obj, "deleteTargetReferences"))};
              })};
}

value EncodeAddReferencesRequest(const AddReferencesRequest& request) {
  return object{{"items",
                 EncodeList(request.items,
                            [](const scada::AddReferencesItem& item) {
                              return object{{"sourceNodeId",
                                             EncodeNodeId(item.source_node_id)},
                                            {"referenceTypeId",
                                             EncodeNodeId(item.reference_type_id)},
                                            {"forward", item.forward},
                                            {"targetServerUri",
                                             item.target_server_uri},
                                            {"targetNodeId",
                                             EncodeExpandedNodeId(item.target_node_id)},
                                            {"targetNodeClass",
                                             EncodeNodeClass(item.target_node_class)}};
                            })}};
}

AddReferencesRequest DecodeAddReferencesRequest(const value& json) {
  return {.items = DecodeList<scada::AddReferencesItem>(
              RequireField(RequireObject(json), "items"), [](const value& entry) {
                const auto& obj = RequireObject(entry);
                return scada::AddReferencesItem{
                    .source_node_id =
                        DecodeNodeId(RequireField(obj, "sourceNodeId")),
                    .reference_type_id =
                        DecodeNodeId(RequireField(obj, "referenceTypeId")),
                    .forward = RequireBool(RequireField(obj, "forward")),
                    .target_server_uri =
                        std::string{RequireString(RequireField(obj, "targetServerUri"))},
                    .target_node_id =
                        DecodeExpandedNodeId(RequireField(obj, "targetNodeId")),
                    .target_node_class =
                        DecodeNodeClass(RequireField(obj, "targetNodeClass"))};
              })};
}

value EncodeDeleteReferencesRequest(const DeleteReferencesRequest& request) {
  return object{{"items",
                 EncodeList(request.items,
                            [](const scada::DeleteReferencesItem& item) {
                              return object{{"sourceNodeId",
                                             EncodeNodeId(item.source_node_id)},
                                            {"referenceTypeId",
                                             EncodeNodeId(item.reference_type_id)},
                                            {"forward", item.forward},
                                            {"targetNodeId",
                                             EncodeExpandedNodeId(item.target_node_id)},
                                            {"deleteBidirectional",
                                             item.delete_bidirectional}};
                            })}};
}

DeleteReferencesRequest DecodeDeleteReferencesRequest(const value& json) {
  return {.items = DecodeList<scada::DeleteReferencesItem>(
              RequireField(RequireObject(json), "items"), [](const value& entry) {
                const auto& obj = RequireObject(entry);
                return scada::DeleteReferencesItem{
                    .source_node_id =
                        DecodeNodeId(RequireField(obj, "sourceNodeId")),
                    .reference_type_id =
                        DecodeNodeId(RequireField(obj, "referenceTypeId")),
                    .forward = RequireBool(RequireField(obj, "forward")),
                    .target_node_id =
                        DecodeExpandedNodeId(RequireField(obj, "targetNodeId")),
                    .delete_bidirectional =
                        RequireBool(RequireField(obj, "deleteBidirectional"))};
              })};
}

value EncodeHistoryReadRawResponse(const HistoryReadRawResponse& response) {
  return object{{"result",
                 object{{"status", EncodeStatus(response.result.status)},
                        {"values", EncodeList(response.result.values, EncodeDataValue)},
                        {"continuationPoint",
                         EncodeByteString(response.result.continuation_point)}}}};
}

HistoryReadRawResponse DecodeHistoryReadRawResponse(const value& json) {
  const auto& result = RequireObject(RequireField(RequireObject(json), "result"));
  return {.result =
              {.status = DecodeStatus(RequireField(result, "status")),
               .values = DecodeList<scada::DataValue>(
                   RequireField(result, "values"), DecodeDataValue),
               .continuation_point =
                   DecodeByteString(RequireField(result, "continuationPoint"))}};
}

value EncodeHistoryReadEventsResponse(const HistoryReadEventsResponse& response) {
  return object{{"result",
                 object{{"status", EncodeStatus(response.result.status)},
                        {"events", EncodeList(response.result.events, EncodeEvent)}}}};
}

HistoryReadEventsResponse DecodeHistoryReadEventsResponse(const value& json) {
  const auto& result = RequireObject(RequireField(RequireObject(json), "result"));
  return {.result =
              {.status = DecodeStatus(RequireField(result, "status")),
               .events = DecodeList<scada::Event>(
                   RequireField(result, "events"), DecodeEvent)}};
}

value EncodeCallResponse(const CallResponse& response) {
  return object{{"results",
                 EncodeList(response.results, [](const CallMethodResult& result) {
                   return object{{"status", EncodeStatus(result.status)}};
                 })}};
}

CallResponse DecodeCallResponse(const value& json) {
  return {.results = DecodeList<CallMethodResult>(
              RequireField(RequireObject(json), "results"), [](const value& entry) {
                return CallMethodResult{
                    .status = DecodeStatus(RequireField(RequireObject(entry), "status"))};
              })};
}

value EncodeAddNodesResponse(const AddNodesResponse& response) {
  return object{{"status", EncodeStatus(response.status)},
                {"results",
                 EncodeList(response.results,
                            [](const scada::AddNodesResult& result) {
                              return object{{"statusCode",
                                             EncodeStatusCode(result.status_code)},
                                            {"addedNodeId",
                                             EncodeNodeId(result.added_node_id)}};
                            })}};
}

AddNodesResponse DecodeAddNodesResponse(const value& json) {
  const auto& obj = RequireObject(json);
  return {.status = DecodeStatus(RequireField(obj, "status")),
          .results = DecodeList<scada::AddNodesResult>(
              RequireField(obj, "results"), [](const value& entry) {
                const auto& result = RequireObject(entry);
                return scada::AddNodesResult{
                    .status_code = DecodeStatusCode(RequireField(result, "statusCode")),
                    .added_node_id = DecodeNodeId(RequireField(result, "addedNodeId"))};
              })};
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

template <class T>
constexpr std::string_view RequestServiceName();

template <>
constexpr std::string_view RequestServiceName<CallRequest>() {
  return "Call";
}
template <>
constexpr std::string_view RequestServiceName<HistoryReadRawRequest>() {
  return "HistoryReadRaw";
}
template <>
constexpr std::string_view RequestServiceName<HistoryReadEventsRequest>() {
  return "HistoryReadEvents";
}
template <>
constexpr std::string_view RequestServiceName<AddNodesRequest>() {
  return "AddNodes";
}
template <>
constexpr std::string_view RequestServiceName<DeleteNodesRequest>() {
  return "DeleteNodes";
}
template <>
constexpr std::string_view RequestServiceName<AddReferencesRequest>() {
  return "AddReferences";
}
template <>
constexpr std::string_view RequestServiceName<DeleteReferencesRequest>() {
  return "DeleteReferences";
}

}  // namespace

boost::json::value EncodeJson(const OpcUaWsServiceRequest& request) {
  return std::visit(
      [](const auto& typed_request) -> value {
        object json;
        json["service"] = RequestServiceName<std::decay_t<decltype(typed_request)>>();
        if constexpr (std::is_same_v<std::decay_t<decltype(typed_request)>,
                                     CallRequest>) {
          json["body"] = EncodeCallRequest(typed_request);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(typed_request)>,
                                            HistoryReadRawRequest>) {
          json["body"] = EncodeHistoryReadRawRequest(typed_request);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(typed_request)>,
                                            HistoryReadEventsRequest>) {
          json["body"] = EncodeHistoryReadEventsRequest(typed_request);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(typed_request)>,
                                            AddNodesRequest>) {
          json["body"] = EncodeAddNodesRequest(typed_request);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(typed_request)>,
                                            DeleteNodesRequest>) {
          json["body"] = EncodeDeleteNodesRequest(typed_request);
        } else if constexpr (std::is_same_v<std::decay_t<decltype(typed_request)>,
                                            AddReferencesRequest>) {
          json["body"] = EncodeAddReferencesRequest(typed_request);
        } else {
          json["body"] = EncodeDeleteReferencesRequest(typed_request);
        }
        return json;
      },
      request);
}

boost::json::value EncodeJson(const OpcUaWsServiceResponse& response) {
  return std::visit(
      [](const auto& typed_response) -> value {
        object json;
        using T = std::decay_t<decltype(typed_response)>;
        if constexpr (std::is_same_v<T, CallResponse>) {
          json["service"] = "Call";
          json["body"] = EncodeCallResponse(typed_response);
        } else if constexpr (std::is_same_v<T, HistoryReadRawResponse>) {
          json["service"] = "HistoryReadRaw";
          json["body"] = EncodeHistoryReadRawResponse(typed_response);
        } else if constexpr (std::is_same_v<T, HistoryReadEventsResponse>) {
          json["service"] = "HistoryReadEvents";
          json["body"] = EncodeHistoryReadEventsResponse(typed_response);
        } else if constexpr (std::is_same_v<T, AddNodesResponse>) {
          json["service"] = "AddNodes";
          json["body"] = EncodeAddNodesResponse(typed_response);
        } else if constexpr (std::is_same_v<T, DeleteNodesResponse>) {
          json["service"] = "DeleteNodes";
          json["body"] = EncodeMultiStatusResponse(typed_response);
        } else if constexpr (std::is_same_v<T, AddReferencesResponse>) {
          json["service"] = "AddReferences";
          json["body"] = EncodeMultiStatusResponse(typed_response);
        } else {
          json["service"] = "DeleteReferences";
          json["body"] = EncodeMultiStatusResponse(typed_response);
        }
        return json;
      },
      response);
}

OpcUaWsServiceRequest DecodeServiceRequest(const boost::json::value& json) {
  const auto& obj = RequireObject(json);
  const auto& body = RequireField(obj, "body");
  auto service = RequireString(RequireField(obj, "service"));
  if (service == "Call")
    return DecodeCallRequest(body);
  if (service == "HistoryReadRaw")
    return DecodeHistoryReadRawRequest(body);
  if (service == "HistoryReadEvents")
    return DecodeHistoryReadEventsRequest(body);
  if (service == "AddNodes")
    return DecodeAddNodesRequest(body);
  if (service == "DeleteNodes")
    return DecodeDeleteNodesRequest(body);
  if (service == "AddReferences")
    return DecodeAddReferencesRequest(body);
  if (service == "DeleteReferences")
    return DecodeDeleteReferencesRequest(body);
  ThrowJsonError("Unknown service request");
}

OpcUaWsServiceResponse DecodeServiceResponse(const boost::json::value& json) {
  const auto& obj = RequireObject(json);
  const auto& body = RequireField(obj, "body");
  auto service = RequireString(RequireField(obj, "service"));
  if (service == "Call")
    return DecodeCallResponse(body);
  if (service == "HistoryReadRaw")
    return DecodeHistoryReadRawResponse(body);
  if (service == "HistoryReadEvents")
    return DecodeHistoryReadEventsResponse(body);
  if (service == "AddNodes")
    return DecodeAddNodesResponse(body);
  if (service == "DeleteNodes")
    return DecodeMultiStatusResponse<DeleteNodesResponse>(body);
  if (service == "AddReferences")
    return DecodeMultiStatusResponse<AddReferencesResponse>(body);
  if (service == "DeleteReferences")
    return DecodeMultiStatusResponse<DeleteReferencesResponse>(body);
  ThrowJsonError("Unknown service response");
}

}  // namespace opcua_ws
