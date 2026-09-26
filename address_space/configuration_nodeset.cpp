#include "address_space/configuration_nodeset.h"

#include "base/base64.h"
#include "base/utf_convert.h"
#include "model/node_id_util.h"
#include "scada/locale_negotiation.h"

#include <pugixml.hpp>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <format>
#include <map>
#include <set>
#include <unordered_map>

namespace scada {
namespace {

// XML namespaces of the UANodeSet schema and of the UA XML value encoding
// (OPC UA Part 6 §F.1, §5.3), as every nodeset in common/model/nodesets/ uses.
constexpr char kNodeSetXmlns[] =
    "http://opcfoundation.org/UA/2011/03/UANodeSet.xsd";
constexpr char kTypesXmlns[] = "http://opcfoundation.org/UA/2008/02/Types.xsd";
// The vendor namespace of this format's Extensions element (Part 6 §F.2:
// "free form XML data that can be used to attach vendor defined data").
constexpr char kExportXmlns[] =
    "http://telecontrol.ru/opcua/configuration-export";
constexpr std::string_view kOpcUaNamespaceUri = "http://opcfoundation.org/UA/";

// Standard namespace-0 NodeIds this format names.
constexpr NumericId kOrganizes = 35;
constexpr NumericId kHasTypeDefinition = 40;
constexpr NumericId kHasProperty = 46;
constexpr NumericId kPropertyType = 68;

bool IsStandard(const NodeId& node_id, NumericId numeric_id) {
  return node_id.namespace_index() == 0 && node_id.is_numeric() &&
         node_id.numeric_id() == numeric_id;
}

// Canonical in-process namespace index <-> the document's NamespaceUris index.
// Index 0 is the OPC UA namespace on both sides (Part 6 §F.2).
class NamespaceTable {
 public:
  explicit NamespaceTable(std::span<const std::string> uris) : uris_{uris} {}

  // Writing: registers `index` as used.
  void Use(NamespaceIndex index) {
    if (index != 0) {
      used_.insert(index);
    }
  }

  // Writing: fixes the document's local indexes, ascending by in-process
  // index so the output is deterministic. False when an index has no URI.
  bool Assign() {
    NamespaceIndex local = 1;
    for (const NamespaceIndex index : used_) {
      if (index >= uris_.size() || uris_[index].empty()) {
        return false;
      }
      to_local_[index] = local++;
    }
    return true;
  }

  NamespaceIndex ToLocal(NamespaceIndex index) const {
    return index == 0 ? 0 : to_local_.at(index);
  }

  const std::set<NamespaceIndex>& used() const { return used_; }

  // Reading: maps the document's local index `local` (1-based position in its
  // NamespaceUris) to the in-process index of `uri`. False when unknown.
  bool AddLocal(std::string_view uri) {
    if (uri == kOpcUaNamespaceUri) {
      from_local_.push_back(0);
      return true;
    }
    const auto i = std::ranges::find(uris_, uri);
    if (i == uris_.end()) {
      return false;
    }
    from_local_.push_back(static_cast<NamespaceIndex>(i - uris_.begin()));
    return true;
  }

  std::optional<NamespaceIndex> FromLocal(NamespaceIndex local) const {
    if (local == 0) {
      return NamespaceIndex{0};
    }
    if (local > from_local_.size()) {
      return std::nullopt;
    }
    return from_local_[local - 1];
  }

 private:
  std::span<const std::string> uris_;
  std::set<NamespaceIndex> used_;
  std::map<NamespaceIndex, NamespaceIndex> to_local_;
  std::vector<NamespaceIndex> from_local_;
};

// --- NodeId, QualifiedName and time text -----------------------------------

NodeId WithNamespace(const NodeId& node_id, NamespaceIndex namespace_index) {
  switch (node_id.type()) {
    case NodeIdType::Numeric:
      return NodeId{node_id.numeric_id(), namespace_index};
    case NodeIdType::String:
      return NodeId{node_id.string_id(), namespace_index};
    case NodeIdType::Opaque:
      return NodeId{node_id.opaque_id(), namespace_index};
    default:
      return {};
  }
}

// The Part 6 §5.1.12 string form, with the document's namespace index.
std::string LocalNodeIdText(const NodeId& node_id,
                            const NamespaceTable& namespaces) {
  return WithNamespace(node_id, namespaces.ToLocal(node_id.namespace_index()))
      .ToString();
}

// "<local ns>:<name>", or just the name in namespace 0 (Part 6 §F.3).
std::string LocalQualifiedNameText(const QualifiedName& name,
                                   const NamespaceTable& namespaces) {
  const NamespaceIndex local = namespaces.ToLocal(name.namespace_index());
  return local == 0 ? name.name() : std::format("{}:{}", local, name.name());
}

using AliasMap = std::unordered_map<std::string, std::string>;

std::optional<NodeId> ParseNodeIdText(std::string_view text,
                                      const NamespaceTable& namespaces,
                                      const AliasMap& aliases) {
  if (const auto alias = aliases.find(std::string{text});
      alias != aliases.end()) {
    text = alias->second;
  }
  NodeId local = NodeId::FromString(text);
  if (local.is_null()) {
    return std::nullopt;
  }
  const auto index = namespaces.FromLocal(local.namespace_index());
  if (!index) {
    return std::nullopt;
  }
  return WithNamespace(local, *index);
}

std::optional<QualifiedName> ParseQualifiedNameText(
    std::string_view text,
    const NamespaceTable& namespaces) {
  NamespaceIndex local = 0;
  if (const auto colon = text.find(':'); colon != std::string_view::npos) {
    unsigned value = 0;
    const auto [end, ec] =
        std::from_chars(text.data(), text.data() + colon, value);
    if (ec == std::errc{} && end == text.data() + colon) {
      local = static_cast<NamespaceIndex>(value);
      text.remove_prefix(colon + 1);
    }
  }
  const auto index = namespaces.FromLocal(local);
  if (!index) {
    return std::nullopt;
  }
  return QualifiedName{std::string{text}, *index};
}

// xs:dateTime in UTC (Part 6 §5.3.1.6: "All DateTime values shall be encoded
// as UTC times"), with the seven fractional digits of the OPC UA DateTime's
// 100 ns resolution; scada::Time is microseconds, so the last is 0. Spelled out
// by calendar arithmetic rather than std::format / std::chrono::parse, whose
// chrono support differs between the standard libraries this tree builds with.
std::string TimeText(Time time) {
  using namespace std::chrono;
  const auto day = floor<days>(time);
  const year_month_day date{day};
  const hh_mm_ss<nanoseconds> clock{duration_cast<nanoseconds>(time - day)};
  const auto ticks = clock.subseconds().count() / 100;
  std::string text = std::format(
      "{:04}-{:02}-{:02}T{:02}:{:02}:{:02}", static_cast<int>(date.year()),
      static_cast<unsigned>(date.month()), static_cast<unsigned>(date.day()),
      clock.hours().count(), clock.minutes().count(), clock.seconds().count());
  if (ticks != 0) {
    text += std::format(".{:07}", ticks);
  }
  return text + "Z";
}

// Parses what TimeText writes: "YYYY-MM-DDTHH:MM:SS[.fraction]Z".
std::optional<Time> ParseTimeText(std::string_view text) {
  using namespace std::chrono;
  const auto number = [&](std::size_t offset, std::size_t length) -> int {
    int value = -1;
    if (offset + length > text.size()) {
      return -1;
    }
    const auto [end, ec] = std::from_chars(
        text.data() + offset, text.data() + offset + length, value);
    return ec == std::errc{} && end == text.data() + offset + length ? value
                                                                     : -1;
  };
  if (text.size() < 20 || text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
      text[13] != ':' || text[16] != ':' || text.back() != 'Z') {
    return std::nullopt;
  }
  const int y = number(0, 4), mo = number(5, 2), d = number(8, 2);
  const int h = number(11, 2), mi = number(14, 2), sec = number(17, 2);
  if (y < 0 || mo < 0 || d < 0 || h < 0 || mi < 0 || sec < 0) {
    return std::nullopt;
  }
  const year_month_day date{year{y}, month{static_cast<unsigned>(mo)},
                            day{static_cast<unsigned>(d)}};
  if (!date.ok()) {
    return std::nullopt;
  }
  nanoseconds fraction{0};
  if (text[19] == '.') {
    const std::string_view digits = text.substr(20, text.size() - 21);
    if (digits.empty() || digits.size() > 9) {
      return std::nullopt;
    }
    long long value = 0;
    for (const char c : digits) {
      if (c < '0' || c > '9') {
        return std::nullopt;
      }
      value = value * 10 + (c - '0');
    }
    for (std::size_t i = digits.size(); i < 9; ++i) {
      value *= 10;
    }
    fraction = nanoseconds{value};
  } else if (text.size() != 20) {
    return std::nullopt;
  }
  const auto point =
      sys_days{date} + hours{h} + minutes{mi} + seconds{sec} + fraction;
  return time_point_cast<Time::duration>(point);
}

// --- Values (Part 6 §5.3) ---------------------------------------------------

// The UA XML element name of `value`'s built-in type, or nullopt when this
// format does not carry it.
std::optional<std::string_view> TypeElementName(const Variant& value) {
  if (!value.is_scalar()) {
    return std::nullopt;
  }
  switch (value.type()) {
    case Variant::BOOL:
      return "Boolean";
    case Variant::INT8:
      return "SByte";
    case Variant::UINT8:
      return "Byte";
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
    case Variant::STRING:
      return "String";
    case Variant::BYTE_STRING:
      return "ByteString";
    case Variant::DATE_TIME:
      return "DateTime";
    case Variant::NODE_ID:
      return "NodeId";
    case Variant::LOCALIZED_TEXT:
      return "LocalizedText";
    case Variant::QUALIFIED_NAME:
      return "QualifiedName";
    default:
      return std::nullopt;
  }
}

// Part 6 §5.3.1.4: the XML floating-point types spell the specials INF, -INF
// and NaN.
std::string DoubleText(double value) {
  if (std::isnan(value)) {
    return "NaN";
  }
  if (std::isinf(value)) {
    return value > 0 ? "INF" : "-INF";
  }
  return std::format("{}", value);
}

template <class T>
std::string IntegerText(const Variant& value) {
  return std::format("{}", value.get<T>());
}

// Appends `value` to `parent` as a UA XML element; false when unsupported.
bool WriteValue(pugi::xml_node parent,
                const Variant& value,
                const NamespaceTable& namespaces) {
  const auto name = TypeElementName(value);
  if (!name) {
    return false;
  }
  pugi::xml_node element =
      parent.append_child(std::format("uax:{}", *name).c_str());
  std::string text;
  switch (value.type()) {
    case Variant::BOOL:
      text = value.get<bool>() ? "true" : "false";
      break;
    case Variant::INT8:
      text = IntegerText<Int8>(value);
      break;
    case Variant::UINT8:
      text = IntegerText<UInt8>(value);
      break;
    case Variant::INT16:
      text = IntegerText<Int16>(value);
      break;
    case Variant::UINT16:
      text = IntegerText<UInt16>(value);
      break;
    case Variant::INT32:
      text = IntegerText<Int32>(value);
      break;
    case Variant::UINT32:
      text = IntegerText<UInt32>(value);
      break;
    case Variant::INT64:
      text = IntegerText<Int64>(value);
      break;
    case Variant::UINT64:
      text = IntegerText<UInt64>(value);
      break;
    case Variant::DOUBLE:
      text = DoubleText(value.get<double>());
      break;
    case Variant::STRING:
      text = value.get<String>();
      break;
    case Variant::BYTE_STRING: {
      const auto& bytes = value.get<ByteString>();
      base::Base64Encode(std::string_view{bytes.data(), bytes.size()}, &text);
      break;
    }
    case Variant::DATE_TIME:
      text = TimeText(value.get<Time>());
      break;
    case Variant::NODE_ID:
      // Part 6 §5.3.1.10: <Identifier> holding the §5.1.12 string.
      element.append_child("uax:Identifier")
          .text()
          .set(LocalNodeIdText(value.get<NodeId>(), namespaces).c_str());
      return true;
    case Variant::LOCALIZED_TEXT: {
      // Part 6 §5.3.1.15: <Locale> and <Text>.
      const auto& localized = value.get<LocalizedText>();
      element.append_child("uax:Locale").text().set(localized.locale.c_str());
      element.append_child("uax:Text")
          .text()
          .set(UtfConvert<char>(localized.text).c_str());
      return true;
    }
    case Variant::QUALIFIED_NAME: {
      // Part 6 §5.3.1.14: <NamespaceIndex> and <Name>.
      const auto& qualified = value.get<QualifiedName>();
      element.append_child("uax:NamespaceIndex")
          .text()
          .set(namespaces.ToLocal(qualified.namespace_index()));
      element.append_child("uax:Name").text().set(qualified.name().c_str());
      return true;
    }
    default:
      return false;
  }
  element.text().set(text.c_str());
  return true;
}

std::string_view LocalName(std::string_view qualified) {
  const auto colon = qualified.find(':');
  return colon == std::string_view::npos ? qualified
                                         : qualified.substr(colon + 1);
}

pugi::xml_node FirstElement(pugi::xml_node parent) {
  for (pugi::xml_node child = parent.first_child(); child;
       child = child.next_sibling()) {
    if (child.type() == pugi::node_element) {
      return child;
    }
  }
  return {};
}

pugi::xml_node ChildByLocalName(pugi::xml_node parent, std::string_view name) {
  for (pugi::xml_node child : parent.children()) {
    if (child.type() == pugi::node_element && LocalName(child.name()) == name) {
      return child;
    }
  }
  return {};
}

template <class T>
std::optional<Variant> ParseInteger(std::string_view text) {
  T value{};
  const auto [end, ec] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || end != text.data() + text.size()) {
    return std::nullopt;
  }
  return Variant{value};
}

std::optional<Variant> ParseDouble(std::string_view text) {
  if (text == "NaN") {
    return Variant{std::nan("")};
  }
  if (text == "INF") {
    return Variant{HUGE_VAL};
  }
  if (text == "-INF") {
    return Variant{-HUGE_VAL};
  }
  double value = 0;
  const auto [end, ec] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  if (ec != std::errc{} || end != text.data() + text.size()) {
    return std::nullopt;
  }
  return Variant{value};
}

// Reads the UA XML element inside a <Value>.
std::optional<Variant> ReadValue(pugi::xml_node value_node,
                                 const NamespaceTable& namespaces,
                                 const AliasMap& aliases) {
  const pugi::xml_node element = FirstElement(value_node);
  if (!element) {
    return std::nullopt;
  }
  const std::string_view type = LocalName(element.name());
  const std::string_view text = element.text().as_string();
  if (type == "Boolean") {
    if (text == "true" || text == "1") {
      return Variant{true};
    }
    if (text == "false" || text == "0") {
      return Variant{false};
    }
    return std::nullopt;
  }
  if (type == "SByte") {
    return ParseInteger<Int8>(text);
  }
  if (type == "Byte") {
    return ParseInteger<UInt8>(text);
  }
  if (type == "Int16") {
    return ParseInteger<Int16>(text);
  }
  if (type == "UInt16") {
    return ParseInteger<UInt16>(text);
  }
  if (type == "Int32") {
    return ParseInteger<Int32>(text);
  }
  if (type == "UInt32") {
    return ParseInteger<UInt32>(text);
  }
  if (type == "Int64") {
    return ParseInteger<Int64>(text);
  }
  if (type == "UInt64") {
    return ParseInteger<UInt64>(text);
  }
  if (type == "Double") {
    return ParseDouble(text);
  }
  if (type == "String") {
    return Variant{String{text}};
  }
  if (type == "ByteString") {
    std::string decoded;
    if (!base::Base64Decode(text, &decoded)) {
      return std::nullopt;
    }
    return Variant{ByteString{decoded.begin(), decoded.end()}};
  }
  if (type == "DateTime") {
    if (const auto time = ParseTimeText(text)) {
      return Variant{*time};
    }
    return std::nullopt;
  }
  if (type == "NodeId") {
    const auto node_id = ParseNodeIdText(
        ChildByLocalName(element, "Identifier").text().as_string(), namespaces,
        aliases);
    return node_id ? std::optional{Variant{*node_id}} : std::nullopt;
  }
  if (type == "LocalizedText") {
    return Variant{LocalizedText{
        ChildByLocalName(element, "Locale").text().as_string(),
        UtfConvert<char16_t>(std::string_view{
            ChildByLocalName(element, "Text").text().as_string()})}};
  }
  if (type == "QualifiedName") {
    const auto index = namespaces.FromLocal(static_cast<NamespaceIndex>(
        ChildByLocalName(element, "NamespaceIndex").text().as_uint()));
    if (!index) {
      return std::nullopt;
    }
    return Variant{QualifiedName{
        ChildByLocalName(element, "Name").text().as_string(), *index}};
  }
  return std::nullopt;
}

// --- Writing ----------------------------------------------------------------

void UseNodeState(const NodeState& node, NamespaceTable& namespaces) {
  namespaces.Use(node.node_id.namespace_index());
  namespaces.Use(node.parent_id.namespace_index());
  namespaces.Use(node.type_definition_id.namespace_index());
  namespaces.Use(node.reference_type_id.namespace_index());
  namespaces.Use(node.attributes.browse_name.namespace_index());
  namespaces.Use(node.attributes.data_type.namespace_index());
  for (const auto& reference : node.references) {
    namespaces.Use(reference.reference_type_id.namespace_index());
    namespaces.Use(reference.node_id.namespace_index());
  }
  const auto use_value = [&](const Variant& value) {
    if (const auto* node_id = value.get_if<NodeId>()) {
      namespaces.Use(node_id->namespace_index());
    } else if (const auto* name = value.get_if<QualifiedName>()) {
      namespaces.Use(name->namespace_index());
    }
  };
  if (node.attributes.value) {
    use_value(*node.attributes.value);
  }
  for (const auto& [prop_decl_id, value] : node.properties) {
    use_value(value);
  }
}

void AppendReference(pugi::xml_node references,
                     const NodeId& reference_type_id,
                     bool forward,
                     const NodeId& target,
                     const NamespaceTable& namespaces) {
  pugi::xml_node reference = references.append_child("Reference");
  reference.append_attribute("ReferenceType")
      .set_value(LocalNodeIdText(reference_type_id, namespaces).c_str());
  if (!forward) {
    reference.append_attribute("IsForward").set_value("false");
  }
  reference.text().set(LocalNodeIdText(target, namespaces).c_str());
}

void AppendDisplayNames(pugi::xml_node element, const LocalizedText& name) {
  // A packed "mul" name carries every translation; UANodeSet lists them as
  // repeated DisplayName elements, one per Locale (Part 6 §F.3).
  for (const LocalizedText& translation : DecodeMultiLanguage(name)) {
    pugi::xml_node display_name = element.append_child("DisplayName");
    if (!translation.locale.empty()) {
      display_name.append_attribute("Locale").set_value(
          translation.locale.c_str());
    }
    display_name.text().set(UtfConvert<char>(translation.text).c_str());
  }
}

Status AppendNode(pugi::xml_node root,
                  const NodeState& node,
                  const NamespaceTable& namespaces,
                  const NodeSetPropertyNames& names) {
  const char* tag = nullptr;
  if (node.node_class == NodeClass::Object) {
    tag = "UAObject";
  } else if (node.node_class == NodeClass::Variable) {
    tag = "UAVariable";
  } else {
    return StatusCode::Bad_NotSupported;
  }

  pugi::xml_node element = root.append_child(tag);
  element.append_attribute("NodeId").set_value(
      LocalNodeIdText(node.node_id, namespaces).c_str());
  element.append_attribute("BrowseName")
      .set_value(LocalQualifiedNameText(node.attributes.browse_name, namespaces)
                     .c_str());
  if (!node.parent_id.is_null()) {
    element.append_attribute("ParentNodeId")
        .set_value(LocalNodeIdText(node.parent_id, namespaces).c_str());
  }
  if (node.node_class == NodeClass::Variable) {
    NodeId data_type = node.attributes.data_type;
    if (data_type.is_null() && node.attributes.value) {
      data_type = ToNodeId(node.attributes.value->type());
    }
    if (!data_type.is_null()) {
      element.append_attribute("DataType")
          .set_value(LocalNodeIdText(data_type, namespaces).c_str());
    }
  }
  AppendDisplayNames(element, node.attributes.display_name);

  // Each property is named first, so a failure writes nothing half-formed.
  struct PropertyNode {
    NodeId node_id;
    QualifiedName browse_name;
    const Variant* value;
  };
  std::vector<PropertyNode> properties;
  for (const auto& [prop_decl_id, value] : node.properties) {
    const auto browse_name =
        names.browse_name
            ? names.browse_name(node.type_definition_id, prop_decl_id)
            : std::nullopt;
    if (!browse_name) {
      return StatusCode::Bad_WrongPropertyId;
    }
    if (!TypeElementName(value)) {
      return StatusCode::Bad_NotSupported;
    }
    properties.push_back({MakeNestedNodeId(node.node_id, browse_name->name()),
                          *browse_name, &value});
  }

  pugi::xml_node references = element.append_child("References");
  if (!node.type_definition_id.is_null()) {
    AppendReference(references, NodeId{kHasTypeDefinition}, true,
                    node.type_definition_id, namespaces);
  }
  if (!node.parent_id.is_null()) {
    const NodeId parent_reference = node.reference_type_id.is_null()
                                        ? NodeId{kOrganizes}
                                        : node.reference_type_id;
    AppendReference(references, parent_reference, false, node.parent_id,
                    namespaces);
  }
  for (const auto& reference : node.references) {
    AppendReference(references, reference.reference_type_id, reference.forward,
                    reference.node_id, namespaces);
  }
  for (const auto& property : properties) {
    AppendReference(references, NodeId{kHasProperty}, true, property.node_id,
                    namespaces);
  }

  if (node.node_class == NodeClass::Variable && node.attributes.value) {
    if (!WriteValue(element.append_child("Value"), *node.attributes.value,
                    namespaces)) {
      return StatusCode::Bad_NotSupported;
    }
  }

  for (const auto& property : properties) {
    pugi::xml_node child = root.append_child("UAVariable");
    child.append_attribute("NodeId").set_value(
        LocalNodeIdText(property.node_id, namespaces).c_str());
    child.append_attribute("BrowseName")
        .set_value(
            LocalQualifiedNameText(property.browse_name, namespaces).c_str());
    child.append_attribute("ParentNodeId")
        .set_value(LocalNodeIdText(node.node_id, namespaces).c_str());
    child.append_attribute("DataType")
        .set_value(LocalNodeIdText(ToNodeId(property.value->type()), namespaces)
                       .c_str());
    child.append_child("DisplayName")
        .text()
        .set(property.browse_name.name().c_str());
    pugi::xml_node child_references = child.append_child("References");
    AppendReference(child_references, NodeId{kHasTypeDefinition}, true,
                    NodeId{kPropertyType}, namespaces);
    AppendReference(child_references, NodeId{kHasProperty}, false, node.node_id,
                    namespaces);
    WriteValue(child.append_child("Value"), *property.value, namespaces);
  }
  return OkStatus();
}

// Orders nodes for a deterministic document.
bool NodeIdLess(const NodeId& a, const NodeId& b) {
  if (a.namespace_index() != b.namespace_index()) {
    return a.namespace_index() < b.namespace_index();
  }
  if (a.type() != b.type()) {
    return a.type() < b.type();
  }
  if (a.is_numeric()) {
    return a.numeric_id() < b.numeric_id();
  }
  return a.ToString() < b.ToString();
}

class StringWriter final : public pugi::xml_writer {
 public:
  void write(const void* data, size_t size) override {
    output.append(static_cast<const char*>(data), size);
  }
  std::string output;
};

// --- Reading ----------------------------------------------------------------

struct ParsedElement {
  pugi::xml_node element;
  NodeState state;
  // Set on a property child: the node it belongs to.
  NodeId property_of;
};

Status ReadElement(pugi::xml_node element,
                   const NamespaceTable& namespaces,
                   const AliasMap& aliases,
                   ParsedElement& parsed) {
  const std::string_view tag = element.name();
  NodeState& state = parsed.state;
  state.node_class =
      tag == "UAVariable" ? NodeClass::Variable : NodeClass::Object;

  const auto node_id = ParseNodeIdText(element.attribute("NodeId").as_string(),
                                       namespaces, aliases);
  const auto browse_name = ParseQualifiedNameText(
      element.attribute("BrowseName").as_string(), namespaces);
  if (!node_id || !browse_name) {
    return StatusCode::Bad_WrongNodeId;
  }
  state.node_id = *node_id;
  state.attributes.browse_name = *browse_name;

  NodeId parent_attribute;
  if (const auto attribute = element.attribute("ParentNodeId")) {
    const auto parent =
        ParseNodeIdText(attribute.as_string(), namespaces, aliases);
    if (!parent) {
      return StatusCode::Bad_WrongNodeId;
    }
    parent_attribute = *parent;
  }
  if (const auto attribute = element.attribute("DataType")) {
    const auto data_type =
        ParseNodeIdText(attribute.as_string(), namespaces, aliases);
    if (!data_type) {
      return StatusCode::Bad_WrongNodeId;
    }
    state.attributes.data_type = *data_type;
  }

  std::vector<LocalizedText> translations;
  for (pugi::xml_node name : element.children("DisplayName")) {
    translations.emplace_back(
        name.attribute("Locale").as_string(),
        UtfConvert<char16_t>(std::string_view{name.text().as_string()}));
  }
  if (translations.size() == 1) {
    state.attributes.display_name = std::move(translations.front());
  } else if (translations.size() > 1) {
    state.attributes.display_name = EncodeMultiLanguage(translations);
  }

  bool is_property = false;
  for (pugi::xml_node reference :
       element.child("References").children("Reference")) {
    const auto reference_type = ParseNodeIdText(
        reference.attribute("ReferenceType").as_string(), namespaces, aliases);
    const auto target =
        ParseNodeIdText(reference.text().as_string(), namespaces, aliases);
    if (!reference_type || !target) {
      return StatusCode::Bad_WrongNodeId;
    }
    const bool forward = reference.attribute("IsForward").as_bool(true);
    if (forward && IsStandard(*reference_type, kHasTypeDefinition)) {
      state.type_definition_id = *target;
      is_property = is_property || IsStandard(*target, kPropertyType);
    } else if (!forward && *target == parent_attribute &&
               state.parent_id.is_null()) {
      state.parent_id = *target;
      state.reference_type_id = *reference_type;
    } else if (forward && IsStandard(*reference_type, kHasProperty)) {
      // The property child itself says whose it is; nothing to keep here.
    } else {
      state.references.push_back({.reference_type_id = *reference_type,
                                  .forward = forward,
                                  .node_id = *target});
    }
  }

  if (const pugi::xml_node value = element.child("Value")) {
    auto parsed_value = ReadValue(value, namespaces, aliases);
    if (!parsed_value) {
      return StatusCode::Bad_CantParseString;
    }
    state.attributes.value = std::move(*parsed_value);
  }

  if (is_property && IsStandard(state.reference_type_id, kHasProperty)) {
    parsed.property_of = state.parent_id;
  }
  return OkStatus();
}

// --- Shared by both documents ------------------------------------------------

constexpr std::string_view kNodeElements[] = {"UAObject", "UAVariable"};

bool IsNodeElement(std::string_view tag) {
  return std::ranges::find(kNodeElements, tag) != std::end(kNodeElements);
}

pugi::xml_node AppendDeclaration(pugi::xml_document& document) {
  pugi::xml_node declaration = document.append_child(pugi::node_declaration);
  declaration.append_attribute("version").set_value("1.0");
  declaration.append_attribute("encoding").set_value("UTF-8");
  return declaration;
}

void AppendNamespaceUris(pugi::xml_node root,
                         const NamespaceTable& namespaces,
                         std::span<const std::string> namespace_uris) {
  pugi::xml_node uris = root.append_child("NamespaceUris");
  for (const NamespaceIndex index : namespaces.used()) {
    uris.append_child("Uri").text().set(namespace_uris[index].c_str());
  }
}

// Appends `nodes` to `container` sorted by NodeId, so equal input gives
// byte-equal output.
Status AppendNodes(pugi::xml_node container,
                   std::span<const NodeState> nodes,
                   const NamespaceTable& namespaces,
                   const NodeSetPropertyNames& names) {
  std::vector<const NodeState*> sorted;
  sorted.reserve(nodes.size());
  for (const NodeState& node : nodes) {
    sorted.push_back(&node);
  }
  std::ranges::sort(sorted, [](const NodeState* a, const NodeState* b) {
    return NodeIdLess(a->node_id, b->node_id);
  });
  for (const NodeState* node : sorted) {
    if (auto status = AppendNode(container, *node, namespaces, names);
        !status) {
      return status;
    }
  }
  return OkStatus();
}

std::string Serialize(const pugi::xml_document& document) {
  StringWriter writer;
  document.save(writer, "  ", pugi::format_default, pugi::encoding_utf8);
  return std::move(writer.output);
}

// The document's NamespaceUris and Aliases, which every NodeId in it is read
// through.
struct DocumentContext {
  NamespaceTable namespaces;
  AliasMap aliases;
};

StatusOr<DocumentContext> ReadDocumentContext(
    pugi::xml_node root,
    std::span<const std::string> namespace_uris) {
  DocumentContext context{NamespaceTable{namespace_uris}, {}};
  for (pugi::xml_node uri : root.child("NamespaceUris").children("Uri")) {
    if (!context.namespaces.AddLocal(uri.text().as_string())) {
      return StatusCode::Bad_WrongNodeId;
    }
  }
  for (pugi::xml_node alias : root.child("Aliases").children("Alias")) {
    context.aliases.emplace(alias.attribute("Alias").as_string(),
                            alias.text().as_string());
  }
  return context;
}

// The Extension children of `root` whose element has local name `name`.
std::vector<pugi::xml_node> FindExtensions(pugi::xml_node root,
                                           std::string_view name) {
  std::vector<pugi::xml_node> found;
  for (pugi::xml_node extension :
       root.child("Extensions").children("Extension")) {
    if (const pugi::xml_node element = ChildByLocalName(extension, name)) {
      found.push_back(element);
    }
  }
  return found;
}

// Reads the UAObject / UAVariable children of `container` and folds each
// property child back into the (declaration id, value) pair its node
// carries. A PropertyType child under HasProperty is a property only when its
// owner is among the nodes read; otherwise it is kept as a node.
StatusOr<std::vector<NodeState>> ReadNodes(pugi::xml_node container,
                                           const DocumentContext& context,
                                           const NodeSetPropertyNames& names) {
  std::vector<ParsedElement> parsed;
  for (pugi::xml_node element : container.children()) {
    if (!IsNodeElement(element.name())) {
      continue;
    }
    ParsedElement& entry = parsed.emplace_back();
    entry.element = element;
    if (auto status =
            ReadElement(element, context.namespaces, context.aliases, entry);
        !status) {
      return status;
    }
  }

  std::unordered_map<std::string, std::size_t> index_of;
  for (std::size_t i = 0; i < parsed.size(); ++i) {
    if (parsed[i].property_of.is_null()) {
      index_of.emplace(parsed[i].state.node_id.ToString(), i);
    }
  }
  for (ParsedElement& entry : parsed) {
    if (entry.property_of.is_null()) {
      continue;
    }
    const auto owner = index_of.find(entry.property_of.ToString());
    if (owner == index_of.end()) {
      entry.property_of = {};
      continue;
    }
    NodeState& node = parsed[owner->second].state;
    const NodeId prop_decl_id =
        names.declaration
            ? names.declaration(node.type_definition_id,
                                entry.state.attributes.browse_name)
            : NodeId{};
    if (prop_decl_id.is_null()) {
      return StatusCode::Bad_WrongPropertyId;
    }
    node.properties.emplace_back(
        prop_decl_id, entry.state.attributes.value.value_or(Variant{}));
  }

  std::vector<NodeState> nodes;
  for (ParsedElement& entry : parsed) {
    if (entry.property_of.is_null()) {
      nodes.push_back(std::move(entry.state));
    }
  }
  return nodes;
}

// --- Changes ----------------------------------------------------------------

void UseReferenceChange(const NodeSetReferenceChange& change,
                        NamespaceTable& namespaces) {
  namespaces.Use(change.source.namespace_index());
  namespaces.Use(change.reference_type_id.namespace_index());
  namespaces.Use(change.target.namespace_index());
}

void AppendReferenceChanges(pugi::xml_node root,
                            const char* name,
                            std::span<const NodeSetReferenceChange> changes,
                            const NamespaceTable& namespaces) {
  if (changes.empty()) {
    return;
  }
  pugi::xml_node list = root.append_child(name);
  for (const NodeSetReferenceChange& change : changes) {
    pugi::xml_node reference = list.append_child("Reference");
    reference.append_attribute("Source").set_value(
        LocalNodeIdText(change.source, namespaces).c_str());
    reference.append_attribute("ReferenceType")
        .set_value(
            LocalNodeIdText(change.reference_type_id, namespaces).c_str());
    if (!change.forward) {
      reference.append_attribute("IsForward").set_value("false");
    }
    reference.text().set(LocalNodeIdText(change.target, namespaces).c_str());
  }
}

StatusOr<std::vector<NodeSetReferenceChange>> ReadReferenceChanges(
    pugi::xml_node list,
    const DocumentContext& context) {
  std::vector<NodeSetReferenceChange> changes;
  for (pugi::xml_node reference : list.children("Reference")) {
    const auto source =
        ParseNodeIdText(reference.attribute("Source").as_string(),
                        context.namespaces, context.aliases);
    const auto reference_type =
        ParseNodeIdText(reference.attribute("ReferenceType").as_string(),
                        context.namespaces, context.aliases);
    const auto target = ParseNodeIdText(reference.text().as_string(),
                                        context.namespaces, context.aliases);
    if (!source || !reference_type || !target) {
      return StatusCode::Bad_WrongNodeId;
    }
    changes.push_back(
        {.source = *source,
         .reference_type_id = *reference_type,
         .forward = reference.attribute("IsForward").as_bool(true),
         .target = *target});
  }
  return changes;
}

void AppendStatusList(pugi::xml_node status_root,
                      const char* name,
                      std::span<const NodeSetOperationStatus> statuses) {
  if (statuses.empty()) {
    return;
  }
  pugi::xml_node list = status_root.append_child(name);
  for (const NodeSetOperationStatus& status : statuses) {
    pugi::xml_node entry = list.append_child("Status");
    if (status.code != 0) {
      entry.append_attribute("Code").set_value(status.code);
    }
    if (!status.details.empty()) {
      entry.text().set(status.details.c_str());
    }
  }
}

std::vector<NodeSetOperationStatus> ReadStatusList(pugi::xml_node list) {
  std::vector<NodeSetOperationStatus> statuses;
  for (pugi::xml_node entry : list.children()) {
    if (LocalName(entry.name()) != "Status") {
      continue;
    }
    statuses.push_back({.code = entry.attribute("Code").as_uint(0),
                        .details = entry.text().as_string()});
  }
  return statuses;
}

// The embedded UANodeSetChangesStatus (§F.22). It is an element of the
// UANodeSet schema's own namespace, which an Extension's `xs:any` admits.
void AppendChangesStatus(pugi::xml_node parent,
                         const NodeSetChangesStatus& status) {
  pugi::xml_node root = parent.append_child("UANodeSetChangesStatus");
  root.append_attribute("xmlns").set_value(kNodeSetXmlns);
  root.append_attribute("LastModified")
      .set_value(TimeText(status.last_modified).c_str());
  root.append_attribute("TransactionId")
      .set_value(status.transaction_id.c_str());
  AppendStatusList(root, "NodesToAdd", status.nodes_to_add);
  AppendStatusList(root, "ReferencesToAdd", status.references_to_add);
  AppendStatusList(root, "NodesToDelete", status.nodes_to_delete);
  AppendStatusList(root, "ReferencesToDelete", status.references_to_delete);
}

NodeSetChangesStatus ReadChangesStatus(pugi::xml_node root) {
  NodeSetChangesStatus status;
  status.transaction_id = root.attribute("TransactionId").as_string();
  if (const auto last_modified =
          ParseTimeText(root.attribute("LastModified").as_string())) {
    status.last_modified = *last_modified;
  }
  status.nodes_to_add = ReadStatusList(ChildByLocalName(root, "NodesToAdd"));
  status.references_to_add =
      ReadStatusList(ChildByLocalName(root, "ReferencesToAdd"));
  status.nodes_to_delete =
      ReadStatusList(ChildByLocalName(root, "NodesToDelete"));
  status.references_to_delete =
      ReadStatusList(ChildByLocalName(root, "ReferencesToDelete"));
  return status;
}

}  // namespace

StatusOr<std::string> WriteConfigurationNodeSet(
    const ConfigurationNodeSet& nodeset,
    std::span<const std::string> namespace_uris,
    const NodeSetPropertyNames& names) {
  NamespaceTable namespaces{namespace_uris};
  for (const NodeState& node : nodeset.nodes) {
    UseNodeState(node, namespaces);
  }
  if (!namespaces.Assign()) {
    return StatusCode::Bad_WrongNodeId;
  }

  pugi::xml_document document;
  AppendDeclaration(document);

  pugi::xml_node root = document.append_child("UANodeSet");
  root.append_attribute("xmlns").set_value(kNodeSetXmlns);
  root.append_attribute("xmlns:uax").set_value(kTypesXmlns);
  root.append_attribute("LastModified")
      .set_value(TimeText(nodeset.last_modified).c_str());

  AppendNamespaceUris(root, namespaces, namespace_uris);

  pugi::xml_node export_info = root.append_child("Extensions")
                                   .append_child("Extension")
                                   .append_child("ConfigurationExport");
  export_info.append_attribute("xmlns").set_value(kExportXmlns);
  export_info.append_attribute("Version").set_value(nodeset.version.c_str());
  for (const std::string& uri : nodeset.scope) {
    export_info.append_child("Scope").text().set(uri.c_str());
  }

  if (auto status = AppendNodes(root, nodeset.nodes, namespaces, names);
      !status) {
    return status;
  }
  return Serialize(document);
}

StatusOr<ConfigurationNodeSet> ReadConfigurationNodeSet(
    std::string_view xml,
    std::span<const std::string> namespace_uris,
    const NodeSetPropertyNames& names) {
  pugi::xml_document document;
  if (!document.load_buffer(xml.data(), xml.size())) {
    return StatusCode::Bad_CantParseString;
  }
  const pugi::xml_node root = document.child("UANodeSet");
  if (!root) {
    return StatusCode::Bad_CantParseString;
  }

  auto context = ReadDocumentContext(root, namespace_uris);
  if (!context.ok()) {
    return context.status();
  }

  ConfigurationNodeSet result;
  if (const auto last_modified =
          ParseTimeText(root.attribute("LastModified").as_string())) {
    result.last_modified = *last_modified;
  }
  for (pugi::xml_node export_info :
       FindExtensions(root, "ConfigurationExport")) {
    result.version = export_info.attribute("Version").as_string();
    for (pugi::xml_node scope : export_info.children()) {
      if (LocalName(scope.name()) == "Scope") {
        result.scope.emplace_back(scope.text().as_string());
      }
    }
  }

  auto nodes = ReadNodes(root, *context, names);
  if (!nodes.ok()) {
    return nodes.status();
  }
  result.nodes = std::move(*nodes);
  return result;
}

StatusOr<std::string> WriteConfigurationNodeSetChanges(
    const ConfigurationNodeSetChanges& changes,
    std::span<const std::string> namespace_uris,
    const NodeSetPropertyNames& names) {
  NamespaceTable namespaces{namespace_uris};
  for (const NodeState& node : changes.nodes_to_add) {
    UseNodeState(node, namespaces);
  }
  for (const auto& change : changes.references_to_add) {
    UseReferenceChange(change, namespaces);
  }
  for (const auto& deletion : changes.nodes_to_delete) {
    namespaces.Use(deletion.node_id.namespace_index());
  }
  for (const auto& change : changes.references_to_delete) {
    UseReferenceChange(change, namespaces);
  }
  if (!namespaces.Assign()) {
    return StatusCode::Bad_WrongNodeId;
  }

  pugi::xml_document document;
  AppendDeclaration(document);

  pugi::xml_node root = document.append_child("UANodeSetChanges");
  root.append_attribute("xmlns").set_value(kNodeSetXmlns);
  root.append_attribute("xmlns:uax").set_value(kTypesXmlns);
  root.append_attribute("LastModified")
      .set_value(TimeText(changes.last_modified).c_str());
  root.append_attribute("TransactionId")
      .set_value(changes.transaction_id.c_str());
  // Part 6 §F.16: "A UANodeSetChanges file is processed as a single
  // operation" — the schema's attribute says so explicitly.
  root.append_attribute("AcceptAllOrNothing").set_value("true");

  AppendNamespaceUris(root, namespaces, namespace_uris);

  pugi::xml_node extensions = root.append_child("Extensions");
  if (!changes.version.empty()) {
    pugi::xml_node export_info = extensions.append_child("Extension")
                                     .append_child("ConfigurationExport");
    export_info.append_attribute("xmlns").set_value(kExportXmlns);
    export_info.append_attribute("Version").set_value(changes.version.c_str());
    for (const std::string& uri : changes.scope) {
      export_info.append_child("Scope").text().set(uri.c_str());
    }
  }
  if (changes.outcome) {
    const ConfigurationImportOutcome& outcome = *changes.outcome;
    pugi::xml_node import_info = extensions.append_child("Extension")
                                     .append_child("ConfigurationImport");
    import_info.append_attribute("xmlns").set_value(kExportXmlns);
    import_info.append_attribute("Committed").set_value(outcome.committed);
    import_info.append_attribute("DryRun").set_value(outcome.dry_run);
    if (!outcome.version.empty()) {
      import_info.append_attribute("Version").set_value(
          outcome.version.c_str());
    }
    AppendChangesStatus(extensions.append_child("Extension"), outcome.status);
  }
  if (!extensions.first_child()) {
    root.remove_child(extensions);
  }

  if (!changes.nodes_to_add.empty()) {
    if (auto status = AppendNodes(root.append_child("NodesToAdd"),
                                  changes.nodes_to_add, namespaces, names);
        !status) {
      return status;
    }
  }
  AppendReferenceChanges(root, "ReferencesToAdd", changes.references_to_add,
                         namespaces);
  if (!changes.nodes_to_delete.empty()) {
    pugi::xml_node list = root.append_child("NodesToDelete");
    for (const NodeSetNodeDeletion& deletion : changes.nodes_to_delete) {
      pugi::xml_node node = list.append_child("Node");
      if (!deletion.delete_reverse_references) {
        node.append_attribute("DeleteReverseReferences").set_value("false");
      }
      node.text().set(LocalNodeIdText(deletion.node_id, namespaces).c_str());
    }
  }
  AppendReferenceChanges(root, "ReferencesToDelete",
                         changes.references_to_delete, namespaces);

  return Serialize(document);
}

StatusOr<ConfigurationNodeSetChanges> ReadConfigurationNodeSetChanges(
    std::string_view xml,
    std::span<const std::string> namespace_uris,
    const NodeSetPropertyNames& names) {
  pugi::xml_document document;
  if (!document.load_buffer(xml.data(), xml.size())) {
    return StatusCode::Bad_CantParseString;
  }
  const pugi::xml_node root = document.child("UANodeSetChanges");
  if (!root) {
    return StatusCode::Bad_CantParseString;
  }

  auto context = ReadDocumentContext(root, namespace_uris);
  if (!context.ok()) {
    return context.status();
  }

  ConfigurationNodeSetChanges result;
  result.transaction_id = root.attribute("TransactionId").as_string();
  if (const auto last_modified =
          ParseTimeText(root.attribute("LastModified").as_string())) {
    result.last_modified = *last_modified;
  }
  for (pugi::xml_node export_info :
       FindExtensions(root, "ConfigurationExport")) {
    result.version = export_info.attribute("Version").as_string();
    for (pugi::xml_node scope : export_info.children()) {
      if (LocalName(scope.name()) == "Scope") {
        result.scope.emplace_back(scope.text().as_string());
      }
    }
  }
  for (pugi::xml_node status_root :
       FindExtensions(root, "UANodeSetChangesStatus")) {
    result.outcome.emplace().status = ReadChangesStatus(status_root);
  }
  for (pugi::xml_node import_info :
       FindExtensions(root, "ConfigurationImport")) {
    if (!result.outcome) {
      result.outcome.emplace();
    }
    result.outcome->committed = import_info.attribute("Committed").as_bool();
    result.outcome->dry_run = import_info.attribute("DryRun").as_bool();
    result.outcome->version = import_info.attribute("Version").as_string();
  }

  auto nodes = ReadNodes(root.child("NodesToAdd"), *context, names);
  if (!nodes.ok()) {
    return nodes.status();
  }
  result.nodes_to_add = std::move(*nodes);

  auto references_to_add =
      ReadReferenceChanges(root.child("ReferencesToAdd"), *context);
  if (!references_to_add.ok()) {
    return references_to_add.status();
  }
  result.references_to_add = std::move(*references_to_add);

  for (pugi::xml_node node : root.child("NodesToDelete").children("Node")) {
    const auto node_id = ParseNodeIdText(node.text().as_string(),
                                         context->namespaces, context->aliases);
    if (!node_id) {
      return StatusCode::Bad_WrongNodeId;
    }
    result.nodes_to_delete.push_back(
        {.node_id = *node_id,
         .delete_reverse_references =
             node.attribute("DeleteReverseReferences").as_bool(true)});
  }

  auto references_to_delete =
      ReadReferenceChanges(root.child("ReferencesToDelete"), *context);
  if (!references_to_delete.ok()) {
    return references_to_delete.status();
  }
  result.references_to_delete = std::move(*references_to_delete);
  return result;
}

std::string NodeSetDocumentKind(std::string_view xml) {
  pugi::xml_document document;
  if (!document.load_buffer(xml.data(), xml.size())) {
    return {};
  }
  for (pugi::xml_node child : document.children()) {
    if (child.type() == pugi::node_element) {
      return std::string{LocalName(child.name())};
    }
  }
  return {};
}

}  // namespace scada
