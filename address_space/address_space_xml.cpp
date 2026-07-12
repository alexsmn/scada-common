#include "address_space/address_space_xml.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/mutable_address_space.h"
#include "address_space/node_factory.h"
#include "address_space/node_utils.h"
#include "common/node_state.h"
#include "common/node_state_util.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "scada/node_attributes.h"

#include <pugixml.hpp>

#include <array>
#include <charconv>
#include <filesystem>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace scada {
namespace {

constexpr char kRootTag[] = "AddressSpace";
constexpr char kNodeTag[] = "Node";
constexpr char kReferenceTag[] = "Reference";
constexpr char kPropertyTag[] = "Property";
constexpr char kValueTag[] = "Value";

std::string ToUtf8(const LocalizedText& text) {
  return ToString(text);
}

LocalizedText FromUtf8(std::string_view text) {
  return ToLocalizedText(text);
}

std::string ToString(NodeClass node_class) {
  switch (node_class) {
    case NodeClass::Object:
      return "Object";
    case NodeClass::Variable:
      return "Variable";
    case NodeClass::Method:
      return "Method";
    case NodeClass::ObjectType:
      return "ObjectType";
    case NodeClass::VariableType:
      return "VariableType";
    case NodeClass::ReferenceType:
      return "ReferenceType";
    case NodeClass::DataType:
      return "DataType";
    case NodeClass::View:
      return "View";
    default:
      return "Unspecified";
  }
}

std::optional<NodeClass> ParseNodeClass(std::string_view text) {
  static constexpr std::pair<std::string_view, NodeClass> kValues[] = {
      {"Object", NodeClass::Object},
      {"Variable", NodeClass::Variable},
      {"Method", NodeClass::Method},
      {"ObjectType", NodeClass::ObjectType},
      {"VariableType", NodeClass::VariableType},
      {"ReferenceType", NodeClass::ReferenceType},
      {"DataType", NodeClass::DataType},
      {"View", NodeClass::View},
  };

  for (auto [name, value] : kValues) {
    if (text == name) {
      return value;
    }
  }
  return std::nullopt;
}

std::string ToString(Variant::Type type) {
  switch (type) {
    case Variant::BOOL:
      return "Bool";
    case Variant::INT8:
      return "Int8";
    case Variant::UINT8:
      return "UInt8";
    case Variant::INT16:
      return "Int16";
    case Variant::UINT16:
      return "UInt16";
    case Variant::INT32:
      return "Int32";
    case Variant::UINT32:
      return "UInt32";
    case Variant::INT64:
      return "Int64";
    case Variant::UINT64:
      return "UInt64";
    case Variant::DOUBLE:
      return "Double";
    case Variant::BYTE_STRING:
      return "ByteString";
    case Variant::STRING:
      return "String";
    case Variant::LOCALIZED_TEXT:
      return "LocalizedText";
    case Variant::NODE_ID:
      return "NodeId";
    case Variant::DATE_TIME:
      return "DateTime";
    default:
      return "Empty";
  }
}

std::optional<Variant::Type> ParseVariantType(std::string_view text) {
  static constexpr std::pair<std::string_view, Variant::Type> kValues[] = {
      {"Bool", Variant::BOOL},
      {"Int8", Variant::INT8},
      {"UInt8", Variant::UINT8},
      {"Int16", Variant::INT16},
      {"UInt16", Variant::UINT16},
      {"Int32", Variant::INT32},
      {"UInt32", Variant::UINT32},
      {"Int64", Variant::INT64},
      {"UInt64", Variant::UINT64},
      {"Double", Variant::DOUBLE},
      {"ByteString", Variant::BYTE_STRING},
      {"String", Variant::STRING},
      {"LocalizedText", Variant::LOCALIZED_TEXT},
      {"NodeId", Variant::NODE_ID},
      {"DateTime", Variant::DATE_TIME},
      {"Empty", Variant::EMPTY},
  };

  for (auto [name, value] : kValues) {
    if (text == name) {
      return value;
    }
  }
  return std::nullopt;
}

NodeId ParseNodeId(std::string_view text) {
  if (text.empty()) {
    return {};
  }
  return NodeIdFromScadaString(text);
}

std::string HexEncode(const ByteString& value) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string result;
  result.reserve(value.size() * 2);
  for (unsigned char byte : value) {
    result.push_back(kHex[byte >> 4]);
    result.push_back(kHex[byte & 0x0F]);
  }
  return result;
}

std::optional<ByteString> HexDecode(std::string_view text) {
  if (text.size() % 2 != 0) {
    return std::nullopt;
  }

  auto hex_value = [](char ch) -> std::optional<char> {
    if (ch >= '0' && ch <= '9') {
      return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
      return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
      return ch - 'a' + 10;
    }
    return std::nullopt;
  };

  ByteString result;
  result.reserve(text.size() / 2);
  for (size_t i = 0; i < text.size(); i += 2) {
    auto high = hex_value(text[i]);
    auto low = hex_value(text[i + 1]);
    if (!high || !low) {
      return std::nullopt;
    }
    result.push_back(static_cast<char>((*high << 4) | *low));
  }
  return result;
}

void SetNodeIdAttribute(pugi::xml_node xml_node,
                        const char* name,
                        const NodeId& node_id) {
  if (!node_id.is_null()) {
    xml_node.append_attribute(name).set_value(NodeIdToScadaString(node_id));
  }
}

void SetQualifiedName(pugi::xml_node xml_node,
                      const char* prefix,
                      const QualifiedName& value) {
  if (value.empty()) {
    return;
  }

  xml_node.append_attribute(std::format("{}Name", prefix).c_str())
      .set_value(value.name());
  if (value.namespace_index() != 0) {
    xml_node.append_attribute(std::format("{}Namespace", prefix).c_str())
        .set_value(value.namespace_index());
  }
}

QualifiedName ReadQualifiedName(pugi::xml_node xml_node, const char* prefix) {
  const auto name_attr =
      xml_node.attribute(std::format("{}Name", prefix).c_str());
  if (!name_attr) {
    return {};
  }

  return {name_attr.as_string(),
          static_cast<NamespaceIndex>(
              xml_node.attribute(std::format("{}Namespace", prefix).c_str())
                  .as_uint())};
}

template <typename T>
void SetIntegerValue(pugi::xml_node xml_node, const T& value) {
  xml_node.text().set(std::format("{}", value).c_str());
}

template <typename T>
std::optional<T> ParseInteger(std::string_view text) {
  T value{};
  auto [ptr, ec] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || ptr != text.data() + text.size()) {
    return std::nullopt;
  }
  return value;
}

template <typename T>
void AppendArrayValues(pugi::xml_node xml_node, const std::vector<T>& values) {
  for (const auto& value : values) {
    auto value_node = xml_node.append_child(kValueTag);
    if constexpr (std::is_same_v<T, LocalizedText>) {
      value_node.text().set(ToUtf8(value).c_str());
    } else if constexpr (std::is_same_v<T, String>) {
      value_node.text().set(value.c_str());
    } else if constexpr (std::is_same_v<T, ByteString>) {
      value_node.text().set(HexEncode(value).c_str());
    } else if constexpr (std::is_same_v<T, NodeId>) {
      value_node.text().set(NodeIdToScadaString(value).c_str());
    } else if constexpr (std::is_same_v<T, DateTime>) {
      SetIntegerValue(value_node, value.ToInternalValue());
    } else {
      SetIntegerValue(value_node, value);
    }
  }
}

Status WriteVariant(pugi::xml_node xml_node, const Variant& value) {
  xml_node.append_attribute("type").set_value(ToString(value.type()).c_str());
  if (value.is_array()) {
    xml_node.append_attribute("array").set_value(true);
  }

  if (value.is_null()) {
    return OkStatus();
  }

  if (!value.is_array()) {
    switch (value.type()) {
      case Variant::BOOL:
        xml_node.text().set(value.as_bool());
        return OkStatus();
      case Variant::INT8:
        SetIntegerValue(xml_node, value.get<Int8>());
        return OkStatus();
      case Variant::UINT8:
        SetIntegerValue(xml_node, value.get<UInt8>());
        return OkStatus();
      case Variant::INT16:
        SetIntegerValue(xml_node, value.get<Int16>());
        return OkStatus();
      case Variant::UINT16:
        SetIntegerValue(xml_node, value.get<UInt16>());
        return OkStatus();
      case Variant::INT32:
        SetIntegerValue(xml_node, value.get<Int32>());
        return OkStatus();
      case Variant::UINT32:
        SetIntegerValue(xml_node, value.get<UInt32>());
        return OkStatus();
      case Variant::INT64:
        SetIntegerValue(xml_node, value.get<Int64>());
        return OkStatus();
      case Variant::UINT64:
        SetIntegerValue(xml_node, value.get<UInt64>());
        return OkStatus();
      case Variant::DOUBLE:
        xml_node.text().set(value.as_double());
        return OkStatus();
      case Variant::BYTE_STRING:
        xml_node.text().set(HexEncode(value.get<ByteString>()).c_str());
        return OkStatus();
      case Variant::STRING:
        xml_node.text().set(value.as_string().c_str());
        return OkStatus();
      case Variant::LOCALIZED_TEXT:
        xml_node.text().set(ToUtf8(value.as_localized_text()).c_str());
        return OkStatus();
      case Variant::NODE_ID:
        xml_node.text().set(NodeIdToScadaString(value.as_node_id()).c_str());
        return OkStatus();
      case Variant::DATE_TIME:
        SetIntegerValue(xml_node, value.get<DateTime>().ToInternalValue());
        return OkStatus();
      default:
        return StatusCode::Bad_WrongTypeId;
    }
  }

  switch (value.type()) {
    case Variant::BYTE_STRING:
      AppendArrayValues(xml_node, value.get<std::vector<ByteString>>());
      return OkStatus();
    case Variant::STRING:
      AppendArrayValues(xml_node, value.get<std::vector<String>>());
      return OkStatus();
    case Variant::LOCALIZED_TEXT:
      AppendArrayValues(xml_node, value.get<std::vector<LocalizedText>>());
      return OkStatus();
    case Variant::NODE_ID:
      AppendArrayValues(xml_node, value.get<std::vector<NodeId>>());
      return OkStatus();
    default:
      return StatusCode::Bad_WrongTypeId;
  }
}

template <typename T>
std::optional<Variant> ParseIntegerVariant(std::string_view text) {
  if (auto value = ParseInteger<T>(text)) {
    return Variant{*value};
  }
  return std::nullopt;
}

std::optional<Variant> ReadScalarVariant(Variant::Type type,
                                         std::string_view text) {
  switch (type) {
    case Variant::EMPTY:
      return Variant{};
    case Variant::BOOL:
      return Variant{text == "true" || text == "1"};
    case Variant::INT8:
      return ParseIntegerVariant<Int8>(text);
    case Variant::UINT8:
      return ParseIntegerVariant<UInt8>(text);
    case Variant::INT16:
      return ParseIntegerVariant<Int16>(text);
    case Variant::UINT16:
      return ParseIntegerVariant<UInt16>(text);
    case Variant::INT32:
      return ParseIntegerVariant<Int32>(text);
    case Variant::UINT32:
      return ParseIntegerVariant<UInt32>(text);
    case Variant::INT64:
      return ParseIntegerVariant<Int64>(text);
    case Variant::UINT64:
      return ParseIntegerVariant<UInt64>(text);
    case Variant::DOUBLE: {
      double value = 0;
      const auto* begin = text.data();
      const auto* end = begin + text.size();
      const auto result = std::from_chars(begin, end, value);
      if (result.ec == std::errc{} && result.ptr == end) {
        return Variant{value};
      }
      return std::nullopt;
    }
    case Variant::BYTE_STRING:
      if (auto value = HexDecode(text)) {
        return Variant{std::move(*value)};
      }
      return std::nullopt;
    case Variant::STRING:
      return Variant{String{text}};
    case Variant::LOCALIZED_TEXT:
      return Variant{FromUtf8(text)};
    case Variant::NODE_ID:
      return Variant{ParseNodeId(text)};
    case Variant::DATE_TIME:
      if (auto value = ParseInteger<int64_t>(text)) {
        return Variant{DateTime::FromInternalValue(*value)};
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<Variant> ReadArrayVariant(Variant::Type type,
                                        pugi::xml_node xml_node) {
  switch (type) {
    case Variant::STRING: {
      std::vector<String> values;
      for (auto value_node : xml_node.children(kValueTag)) {
        values.emplace_back(value_node.text().as_string());
      }
      return Variant{std::move(values)};
    }
    case Variant::BYTE_STRING: {
      std::vector<ByteString> values;
      for (auto value_node : xml_node.children(kValueTag)) {
        auto value = HexDecode(value_node.text().as_string());
        if (!value) {
          return std::nullopt;
        }
        values.emplace_back(std::move(*value));
      }
      return Variant{std::move(values)};
    }
    case Variant::LOCALIZED_TEXT: {
      std::vector<LocalizedText> values;
      for (auto value_node : xml_node.children(kValueTag)) {
        values.emplace_back(FromUtf8(value_node.text().as_string()));
      }
      return Variant{std::move(values)};
    }
    case Variant::NODE_ID: {
      std::vector<NodeId> values;
      for (auto value_node : xml_node.children(kValueTag)) {
        values.emplace_back(ParseNodeId(value_node.text().as_string()));
      }
      return Variant{std::move(values)};
    }
    default:
      return std::nullopt;
  }
}

Status ReadVariant(pugi::xml_node xml_node, Variant& value) {
  auto type = ParseVariantType(xml_node.attribute("type").as_string());
  if (!type) {
    return StatusCode::Bad_WrongTypeId;
  }

  std::optional<Variant> parsed =
      xml_node.attribute("array").as_bool()
          ? ReadArrayVariant(*type, xml_node)
          : ReadScalarVariant(*type, xml_node.text().as_string());
  if (!parsed) {
    return StatusCode::Bad_CantParseString;
  }

  value = std::move(*parsed);
  return OkStatus();
}

Status WriteNodeState(pugi::xml_node parent, const NodeState& node_state) {
  auto node = parent.append_child(kNodeTag);
  node.append_attribute("id").set_value(
      NodeIdToScadaString(node_state.node_id));
  node.append_attribute("class").set_value(
      ToString(node_state.node_class).c_str());
  SetNodeIdAttribute(node, "typeDefinition", node_state.type_definition_id);
  SetNodeIdAttribute(node, "parent", node_state.parent_id);
  SetNodeIdAttribute(node, "parentReference", node_state.reference_type_id);
  SetNodeIdAttribute(node, "supertype", node_state.supertype_id);

  SetQualifiedName(node, "browse", node_state.attributes.browse_name);
  if (!node_state.attributes.display_name.empty()) {
    node.append_attribute("displayName")
        .set_value(ToUtf8(node_state.attributes.display_name).c_str());
  }
  SetNodeIdAttribute(node, "dataType", node_state.attributes.data_type);
  if (node_state.attributes.value) {
    auto status = WriteVariant(node.append_child("AttributeValue"),
                               *node_state.attributes.value);
    if (!status) {
      return status;
    }
  }

  for (const auto& property : node_state.properties) {
    auto property_node = node.append_child(kPropertyTag);
    SetNodeIdAttribute(property_node, "id", property.first);
    auto status = WriteVariant(property_node, property.second);
    if (!status) {
      return status;
    }
  }

  for (const auto& reference : node_state.references) {
    auto reference_node = node.append_child(kReferenceTag);
    SetNodeIdAttribute(reference_node, "type", reference.reference_type_id);
    reference_node.append_attribute("forward").set_value(reference.forward);
    SetNodeIdAttribute(reference_node, "target", reference.node_id);
    reference_node.append_attribute("targetClass")
        .set_value(ToString(reference.node_class).c_str());
  }

  return OkStatus();
}

Status ReadNodeState(pugi::xml_node node, NodeState& node_state) {
  node_state.node_id = ParseNodeId(node.attribute("id").as_string());
  if (node_state.node_id.is_null()) {
    return StatusCode::Bad_WrongNodeId;
  }

  auto node_class = ParseNodeClass(node.attribute("class").as_string());
  if (!node_class) {
    return StatusCode::Bad_WrongNodeClass;
  }
  node_state.node_class = *node_class;

  node_state.type_definition_id =
      ParseNodeId(node.attribute("typeDefinition").as_string());
  node_state.parent_id = ParseNodeId(node.attribute("parent").as_string());
  node_state.reference_type_id =
      ParseNodeId(node.attribute("parentReference").as_string());
  node_state.supertype_id =
      ParseNodeId(node.attribute("supertype").as_string());
  node_state.attributes.browse_name = ReadQualifiedName(node, "browse");
  node_state.attributes.display_name =
      FromUtf8(node.attribute("displayName").as_string());
  node_state.attributes.data_type =
      ParseNodeId(node.attribute("dataType").as_string());

  if (auto value_node = node.child("AttributeValue")) {
    Variant value;
    auto status = ReadVariant(value_node, value);
    if (!status) {
      return status;
    }
    node_state.attributes.value = std::move(value);
  }

  for (auto property_node : node.children(kPropertyTag)) {
    auto property_id = ParseNodeId(property_node.attribute("id").as_string());
    if (property_id.is_null()) {
      return StatusCode::Bad_WrongPropertyId;
    }

    Variant value;
    auto status = ReadVariant(property_node, value);
    if (!status) {
      return status;
    }
    node_state.properties.emplace_back(std::move(property_id),
                                       std::move(value));
  }

  for (auto reference_node : node.children(kReferenceTag)) {
    auto target_class =
        ParseNodeClass(reference_node.attribute("targetClass").as_string());
    if (!target_class) {
      return StatusCode::Bad_WrongNodeClass;
    }

    node_state.references.emplace_back(ReferenceDescription{
        .reference_type_id =
            ParseNodeId(reference_node.attribute("type").as_string()),
        .forward = reference_node.attribute("forward").as_bool(true),
        .node_id = ParseNodeId(reference_node.attribute("target").as_string()),
        .node_class = *target_class});
  }

  return OkStatus();
}

// Reads every <Node> element from a parsed document, appending the decoded
// NodeStates to `out`. Multiple documents can be parsed into a single vector so
// they are materialized together (see ApplyNodeStates /
// LoadStaticAddressSpace).
Status ParseNodeStates(const pugi::xml_document& document,
                       std::vector<NodeState>& out) {
  auto root = document.child(kRootTag);
  if (!root) {
    return StatusCode::Bad_CantParseString;
  }

  for (auto node : root.children(kNodeTag)) {
    auto& node_state = out.emplace_back();
    auto status = ReadNodeState(node, node_state);
    if (!status) {
      return status;
    }
  }
  return OkStatus();
}

// Materializes parsed NodeStates into the address space: first creates every
// node, then resolves supertype and reference links. Resolution runs only after
// all nodes exist, so references may target nodes parsed from any input file —
// this lets a static address space be split across multiple files without
// regard to inter-file ordering.
Status ApplyNodeStates(std::vector<NodeState> node_states,
                       MutableAddressSpace& address_space,
                       NodeFactory& node_factory) {
  SortNodesHierarchically(node_states);

  for (const auto& node_state : node_states) {
    if (address_space.GetNode(node_state.node_id)) {
      continue;
    }

    NodeState create_state = node_state;
    create_state.properties.clear();
    auto [status, node] = node_factory.CreateNode(create_state);
    if (!status) {
      return status;
    }
  }

  for (const auto& node_state : node_states) {
    if (!node_state.supertype_id.is_null()) {
      auto* supertype = address_space.GetNode(node_state.supertype_id);
      if (!supertype) {
        return StatusCode::Bad_WrongTypeId;
      }
      if (!FindReference(*supertype, id::HasSubtype, true,
                         node_state.node_id)) {
        AddReference(address_space, id::HasSubtype, node_state.supertype_id,
                     node_state.node_id);
      }
    }

    for (const auto& reference : node_state.references) {
      const auto& source_id =
          reference.forward ? node_state.node_id : reference.node_id;
      const auto& target_id =
          reference.forward ? reference.node_id : node_state.node_id;
      auto* source = address_space.GetNode(source_id);
      if (!source) {
        return StatusCode::Bad_WrongNodeId;
      }
      if (!FindReference(*source, reference.reference_type_id, true,
                         target_id)) {
        AddReference(address_space, reference.reference_type_id, source_id,
                     target_id);
      }
    }
  }

  return OkStatus();
}

Status LoadAddressSpaceXmlDocument(const pugi::xml_document& document,
                                   MutableAddressSpace& address_space,
                                   NodeFactory& node_factory) {
  std::vector<NodeState> node_states;
  if (auto status = ParseNodeStates(document, node_states); !status) {
    return status;
  }
  return ApplyNodeStates(std::move(node_states), address_space, node_factory);
}

// ---------------------------------------------------------------------------
// OPC UA UANodeSet2 parsing (http://opcfoundation.org/UA/2011/03/UANodeSet.xsd,
// OPC UA Part 6). Produces the same NodeState vector as the custom parser above
// so materialization (ApplyNodeStates) is shared. See
// common/model/docs/uanodeset-migration.md.
// ---------------------------------------------------------------------------

constexpr char kUaRootTag[] = "UANodeSet";

// Standard namespace-0 hierarchical/type reference-type numeric ids, used to
// reverse a node's <References> back into the NodeState hierarchy fields.
constexpr NumericId kUaOrganizes = 35;
constexpr NumericId kUaHasTypeDefinition = 40;
constexpr NumericId kUaHasSubtype = 45;
constexpr NumericId kUaHasProperty = 46;
constexpr NumericId kUaHasComponent = 47;

std::optional<NodeClass> ParseUaElementClass(std::string_view tag) {
  static constexpr std::pair<std::string_view, NodeClass> kValues[] = {
      {"UAObject", NodeClass::Object},
      {"UAVariable", NodeClass::Variable},
      {"UAMethod", NodeClass::Method},
      {"UAView", NodeClass::View},
      {"UAObjectType", NodeClass::ObjectType},
      {"UAVariableType", NodeClass::VariableType},
      {"UADataType", NodeClass::DataType},
      {"UAReferenceType", NodeClass::ReferenceType},
  };
  for (auto [name, value] : kValues) {
    if (tag == name) {
      return value;
    }
  }
  return std::nullopt;
}

// Maps a UANodeSet file-local namespace index to the server-global index. Local
// index 0 is always OPC UA namespace 0. Any listed vendor URI maps to the SCADA
// namespace; a real URI->index table is a Phase 0 item (see the migration doc).
using UaNamespaceMap = std::vector<NamespaceIndex>;

UaNamespaceMap ReadNamespaceMap(pugi::xml_node root) {
  UaNamespaceMap map;
  map.push_back(0);  // local index 0 == OPC UA (http://opcfoundation.org/UA/)
  for (auto uri : root.child("NamespaceUris").children("Uri")) {
    std::string_view text = uri.text().as_string();
    map.push_back(text == "http://opcfoundation.org/UA/"
                      ? NamespaceIndex{0}
                      : NamespaceIndexes::SCADA);
  }
  return map;
}

using UaAliasMap = std::unordered_map<std::string, std::string>;

UaAliasMap ReadAliasMap(pugi::xml_node root) {
  UaAliasMap map;
  for (auto alias : root.child("Aliases").children("Alias")) {
    map.emplace(alias.attribute("Alias").as_string(), alias.text().as_string());
  }
  return map;
}

// Parses a UANodeSet NodeId ("ns=N;i=M", "i=M", or an alias name) into an
// internal NodeId with the global namespace index. Only numeric identifiers are
// used by the model.
NodeId ParseUaNodeId(std::string_view text,
                     const UaNamespaceMap& ns_map,
                     const UaAliasMap& aliases) {
  if (text.empty()) {
    return {};
  }
  if (auto it = aliases.find(std::string{text}); it != aliases.end()) {
    text = it->second;
  }

  NamespaceIndex local_ns = 0;
  if (text.starts_with("ns=")) {
    text.remove_prefix(3);
    unsigned value = 0;
    auto [ptr, ec] =
        std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr == text.data() || *ptr != ';') {
      return {};
    }
    local_ns = static_cast<NamespaceIndex>(value);
    text = text.substr(static_cast<size_t>(ptr - text.data()) + 1);
  }
  if (!text.starts_with("i=")) {
    return {};  // string/guid/opaque identifiers are unused by the model
  }
  text.remove_prefix(2);
  auto numeric = ParseInteger<NumericId>(text);
  if (!numeric) {
    return {};
  }
  const NamespaceIndex global_ns =
      local_ns < ns_map.size() ? ns_map[local_ns] : local_ns;
  return NodeId{*numeric, global_ns};
}

QualifiedName ParseUaBrowseName(std::string_view text,
                                const UaNamespaceMap& ns_map) {
  NamespaceIndex ns = 0;
  if (auto pos = text.find(':'); pos != std::string_view::npos) {
    if (auto local = ParseInteger<unsigned>(text.substr(0, pos))) {
      ns = *local < ns_map.size() ? ns_map[*local]
                                  : static_cast<NamespaceIndex>(*local);
    }
    text = text.substr(pos + 1);
  }
  return {std::string{text}, ns};
}

std::optional<Variant::Type> ParseUaxType(std::string_view name) {
  static constexpr std::pair<std::string_view, Variant::Type> kValues[] = {
      {"Boolean", Variant::BOOL},    {"SByte", Variant::INT8},
      {"Byte", Variant::UINT8},      {"Int16", Variant::INT16},
      {"UInt16", Variant::UINT16},   {"Int32", Variant::INT32},
      {"UInt32", Variant::UINT32},   {"Int64", Variant::INT64},
      {"UInt64", Variant::UINT64},   {"Double", Variant::DOUBLE},
      {"String", Variant::STRING},   {"DateTime", Variant::DATE_TIME},
      {"ByteString", Variant::BYTE_STRING},
      {"NodeId", Variant::NODE_ID},
      {"LocalizedText", Variant::LOCALIZED_TEXT},
  };
  for (auto [n, v] : kValues) {
    if (name == n) {
      return v;
    }
  }
  return std::nullopt;
}

// Decodes a UANodeSet <Value> holding a scalar uax: element, e.g.
// <Value><uax:Int32>0</uax:Int32></Value>.
bool ReadUaValue(pugi::xml_node value_node, Variant& out) {
  auto element = value_node.first_child();
  while (element && element.type() != pugi::node_element) {
    element = element.next_sibling();
  }
  if (!element) {
    return false;
  }
  std::string_view local = element.name();
  if (auto pos = local.find(':'); pos != std::string_view::npos) {
    local = local.substr(pos + 1);
  }
  auto type = ParseUaxType(local);
  if (!type) {
    return false;
  }
  auto parsed = ReadScalarVariant(*type, element.text().as_string());
  if (!parsed) {
    return false;
  }
  out = std::move(*parsed);
  return true;
}

Status ReadUaNodeState(pugi::xml_node node,
                       const UaNamespaceMap& ns_map,
                       const UaAliasMap& aliases,
                       NodeState& node_state) {
  auto node_class = ParseUaElementClass(node.name());
  if (!node_class) {
    return StatusCode::Bad_WrongNodeClass;
  }
  node_state.node_class = *node_class;

  node_state.node_id =
      ParseUaNodeId(node.attribute("NodeId").as_string(), ns_map, aliases);
  if (node_state.node_id.is_null()) {
    return StatusCode::Bad_WrongNodeId;
  }

  node_state.attributes.browse_name =
      ParseUaBrowseName(node.attribute("BrowseName").as_string(), ns_map);
  node_state.attributes.display_name =
      FromUtf8(node.child("DisplayName").text().as_string());
  if (auto data_type = node.attribute("DataType")) {
    node_state.attributes.data_type =
        ParseUaNodeId(data_type.as_string(), ns_map, aliases);
  }
  if (auto value_node = node.child("Value")) {
    Variant value;
    if (ReadUaValue(value_node, value)) {
      node_state.attributes.value = std::move(value);
    }
  }

  // Reverse the reference set into the NodeState hierarchy fields (matching the
  // custom parser's shape), leaving everything else in `references`.
  for (auto reference : node.child("References").children("Reference")) {
    NodeId reference_type_id = ParseUaNodeId(
        reference.attribute("ReferenceType").as_string(), ns_map, aliases);
    NodeId target =
        ParseUaNodeId(reference.child_value(), ns_map, aliases);
    const bool forward = reference.attribute("IsForward").as_bool(true);

    const NumericId ref_num =
        (reference_type_id.namespace_index() == 0 &&
         reference_type_id.type() == NodeIdType::Numeric)
            ? reference_type_id.numeric_id()
            : 0;

    if (!forward && ref_num == kUaHasSubtype) {
      node_state.supertype_id = target;
      node_state.parent_id = target;
      node_state.reference_type_id = std::move(reference_type_id);
    } else if (!forward && (ref_num == kUaHasComponent ||
                            ref_num == kUaHasProperty ||
                            ref_num == kUaOrganizes)) {
      node_state.parent_id = target;
      node_state.reference_type_id = std::move(reference_type_id);
    } else if (forward && ref_num == kUaHasTypeDefinition) {
      node_state.type_definition_id = target;
    } else {
      node_state.references.emplace_back(ReferenceDescription{
          .reference_type_id = std::move(reference_type_id),
          .forward = forward,
          .node_id = std::move(target),
          .node_class = NodeClass::Object});
    }
  }
  return OkStatus();
}

Status ParseUaNodeStates(const pugi::xml_document& document,
                         std::vector<NodeState>& out) {
  auto root = document.child(kUaRootTag);
  if (!root) {
    return StatusCode::Bad_CantParseString;
  }
  const UaNamespaceMap ns_map = ReadNamespaceMap(root);
  const UaAliasMap aliases = ReadAliasMap(root);

  for (auto node : root.children()) {
    if (!ParseUaElementClass(node.name())) {
      continue;  // NamespaceUris, Models, Aliases, etc.
    }
    auto& node_state = out.emplace_back();
    if (auto status = ReadUaNodeState(node, ns_map, aliases, node_state);
        !status) {
      return status;
    }
  }
  return OkStatus();
}

}  // namespace

Status LoadAddressSpaceXml(const std::filesystem::path& path,
                           MutableAddressSpace& address_space,
                           NodeFactory& node_factory) {
  pugi::xml_document document;
  const auto path_string = path.string();
  auto result = document.load_file(path_string.c_str());
  if (!result) {
    return StatusCode::Bad_CantParseString;
  }

  return LoadAddressSpaceXmlDocument(document, address_space, node_factory);
}

Status LoadStaticAddressSpace(std::span<const std::filesystem::path> paths,
                              MutableAddressSpace& address_space,
                              NodeFactory& node_factory) {
  // Parse every file into one NodeState set, then materialize it once so
  // references that cross file boundaries (e.g. a protocol type whose supertype
  // lives in another partition) resolve regardless of load order.
  std::vector<NodeState> node_states;
  for (const auto& path : paths) {
    pugi::xml_document document;
    const auto path_string = path.string();
    if (!document.load_file(path_string.c_str())) {
      return StatusCode::Bad_CantParseString;
    }
    if (auto status = ParseNodeStates(document, node_states); !status) {
      return status;
    }
  }
  return ApplyNodeStates(std::move(node_states), address_space, node_factory);
}

Status LoadUANodeSetXml(const std::filesystem::path& path,
                        MutableAddressSpace& address_space,
                        NodeFactory& node_factory) {
  pugi::xml_document document;
  const auto path_string = path.string();
  if (!document.load_file(path_string.c_str())) {
    return StatusCode::Bad_CantParseString;
  }
  std::vector<NodeState> node_states;
  if (auto status = ParseUaNodeStates(document, node_states); !status) {
    return status;
  }
  return ApplyNodeStates(std::move(node_states), address_space, node_factory);
}

Status LoadStaticUANodeSet(std::span<const std::filesystem::path> paths,
                           MutableAddressSpace& address_space,
                           NodeFactory& node_factory) {
  std::vector<NodeState> node_states;
  for (const auto& path : paths) {
    pugi::xml_document document;
    const auto path_string = path.string();
    if (!document.load_file(path_string.c_str())) {
      return StatusCode::Bad_CantParseString;
    }
    if (auto status = ParseUaNodeStates(document, node_states); !status) {
      return status;
    }
  }
  return ApplyNodeStates(std::move(node_states), address_space, node_factory);
}

Status LoadStaticNodesets(std::span<const std::filesystem::path> paths,
                          MutableAddressSpace& address_space,
                          NodeFactory& node_factory) {
  // Parse each file with the parser matching its root element, accumulating into
  // one NodeState set so references resolve across files (and across formats)
  // regardless of order -- this is what lets custom and UANodeSet2 files coexist
  // during the migration.
  std::vector<NodeState> node_states;
  for (const auto& path : paths) {
    pugi::xml_document document;
    const auto path_string = path.string();
    if (!document.load_file(path_string.c_str())) {
      return StatusCode::Bad_CantParseString;
    }
    const Status status = document.child(kUaRootTag)
                              ? ParseUaNodeStates(document, node_states)
                              : ParseNodeStates(document, node_states);
    if (!status) {
      return status;
    }
  }
  return ApplyNodeStates(std::move(node_states), address_space, node_factory);
}

Status SaveAddressSpaceXml(const std::filesystem::path& path,
                           const AddressSpace& address_space) {
  pugi::xml_document document;
  auto declaration = document.append_child(pugi::node_declaration);
  declaration.append_attribute("version").set_value("1.0");
  declaration.append_attribute("encoding").set_value("UTF-8");

  auto root = document.append_child(kRootTag);
  root.append_attribute("format").set_value("scada-node-state-v1");

  std::vector<NodeState> node_states;
  if (auto* address_space_impl =
          dynamic_cast<const AddressSpaceImpl*>(&address_space)) {
    for (const auto& [node_id, node] : address_space_impl->node_map()) {
      if (node) {
        node_states.emplace_back(MakeNodeState(*node));
      }
    }
  } else {
    node_states = MakeNodeStates(address_space);
  }
  std::ranges::sort(node_states, {}, [](const NodeState& node_state) {
    return NodeIdToScadaString(node_state.node_id);
  });

  for (const auto& node_state : node_states) {
    auto status = WriteNodeState(root, node_state);
    if (!status) {
      return status;
    }
  }

  std::filesystem::create_directories(path.parent_path());
  const auto path_string = path.string();
  if (!document.save_file(path_string.c_str(), "  ")) {
    return StatusCode::Bad;
  }
  return OkStatus();
}

}  // namespace scada
