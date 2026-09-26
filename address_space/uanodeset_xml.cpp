#include "address_space/uanodeset_xml.h"

#include "base/base64.h"
#include "base/utf_convert.h"
#include "scada/locale_negotiation.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <format>

namespace scada::uanodeset {
namespace {

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

// Part 6 §5.3.1.6's written forms of the earliest and latest DateTime.
constexpr std::string_view kEarliestTimeText = "0001-01-01T00:00:00Z";
constexpr std::string_view kLatestTimeText = "9999-12-31T23:59:59Z";

}  // namespace

// NamespaceTable

void NamespaceTable::Use(NamespaceIndex index) {
  if (index != 0) {
    used_.insert(index);
  }
}

bool NamespaceTable::Assign() {
  NamespaceIndex local = 1;
  for (const NamespaceIndex index : used_) {
    if (index >= uris_.size() || uris_[index].empty()) {
      return false;
    }
    to_local_[index] = local++;
  }
  return true;
}

NamespaceIndex NamespaceTable::ToLocal(NamespaceIndex index) const {
  return index == 0 ? 0 : to_local_.at(index);
}

bool NamespaceTable::AddLocal(std::string_view uri) {
  if (uri == kOpcUaNamespaceUri) {
    from_local_.push_back(0);
    return true;
  }
  if (const auto i = std::ranges::find(uris_, uri); i != uris_.end()) {
    from_local_.push_back(static_cast<NamespaceIndex>(i - uris_.begin()));
    return true;
  }
  if (fallback_) {
    from_local_.push_back(*fallback_);
    return true;
  }
  return false;
}

std::optional<NamespaceIndex> NamespaceTable::FromLocal(
    NamespaceIndex local) const {
  if (local == 0) {
    return NamespaceIndex{0};
  }
  if (local > from_local_.size()) {
    return std::nullopt;
  }
  return from_local_[local - 1];
}

bool NamespaceTable::AddLocals(pugi::xml_node root) {
  for (pugi::xml_node uri : root.child("NamespaceUris").children("Uri")) {
    if (!AddLocal(uri.text().as_string())) {
      return false;
    }
  }
  return true;
}

AliasMap ReadAliases(pugi::xml_node root) {
  AliasMap aliases;
  for (pugi::xml_node alias : root.child("Aliases").children("Alias")) {
    aliases.emplace(alias.attribute("Alias").as_string(),
                    alias.text().as_string());
  }
  return aliases;
}

// --- NodeId, QualifiedName and time text -----------------------------------

std::string LocalNodeIdText(const NodeId& node_id,
                            const NamespaceTable& namespaces) {
  return WithNamespace(node_id, namespaces.ToLocal(node_id.namespace_index()))
      .ToString();
}

std::string LocalQualifiedNameText(const QualifiedName& name,
                                   const NamespaceTable& namespaces) {
  const NamespaceIndex local = namespaces.ToLocal(name.namespace_index());
  return local == 0 ? name.name() : std::format("{}:{}", local, name.name());
}

std::optional<NodeId> ParseNodeIdText(std::string_view text,
                                      const NamespaceTable& namespaces,
                                      const AliasMap& aliases) {
  if (const auto alias = aliases.find(std::string{text});
      alias != aliases.end()) {
    text = alias->second;
  }
  // Part 6 §5.1.12's form names its identifier type, "ns=<n>;" aside; a bare
  // word would otherwise read as a String id, which is how an alias a
  // document forgot to declare slipped through as a NodeId nobody meant.
  const std::string_view identifier =
      text.starts_with("ns=") && text.find(';') != std::string_view::npos
          ? text.substr(text.find(';') + 1)
          : text;
  if (identifier.size() < 2 || identifier[1] != '=' ||
      std::string_view{"isgb"}.find(identifier[0]) == std::string_view::npos) {
    return std::nullopt;
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

// Spelled out by calendar arithmetic rather than std::format /
// std::chrono::parse, whose chrono support differs between the standard
// libraries this tree builds with.
std::string TimeText(Time time) {
  using namespace std::chrono;
  if (time <= kNullTime) {
    return std::string{kEarliestTimeText};
  }
  // Tested before any calendar arithmetic, which Time::max() would overflow.
  if (time == kMaxTime) {
    return std::string{kLatestTimeText};
  }
  const auto day = floor<days>(time);
  const year_month_day date{day};
  if (date.year() > year{9999}) {
    return std::string{kLatestTimeText};
  }
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
      text[13] != ':' || text[16] != ':') {
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

  // The fraction, then the zone: "Z" or "+HH:MM" / "-HH:MM".
  std::size_t position = 19;
  // Microseconds, Time's own resolution: nanoseconds span only ±292 years
  // around 1970, and the null DateTime's written form is year 1.
  microseconds fraction{0};
  if (position < text.size() && text[position] == '.') {
    ++position;
    long long value = 0;
    std::size_t digits = 0;
    for (; position < text.size() && text[position] >= '0' &&
           text[position] <= '9';
         ++position, ++digits) {
      if (digits < 9) {
        value = value * 10 + (text[position] - '0');
      }
    }
    if (digits == 0) {
      return std::nullopt;
    }
    for (std::size_t i = digits; i < 9; ++i) {
      value *= 10;
    }
    fraction = duration_cast<microseconds>(nanoseconds{value});
  }
  minutes offset{0};
  if (position + 1 == text.size() && text[position] == 'Z') {
    // UTC.
  } else if (position + 6 == text.size() &&
             (text[position] == '+' || text[position] == '-') &&
             text[position + 3] == ':') {
    const int offset_hours = number(position + 1, 2);
    const int offset_minutes = number(position + 4, 2);
    if (offset_hours < 0 || offset_minutes < 0) {
      return std::nullopt;
    }
    offset = hours{offset_hours} + minutes{offset_minutes};
    if (text[position] == '-') {
      offset = -offset;
    }
  } else {
    // §5.3.1.6 lists a zoneless value as incorrect.
    return std::nullopt;
  }

  if (text.substr(0, position) >= kLatestTimeText.substr(0, 19) &&
      offset == minutes{0}) {
    return kMaxTime;
  }
  const auto point = sys_days{date} + hours{h} + minutes{mi} + seconds{sec} +
                     fraction - offset;
  const Time time = time_point_cast<Time::duration>(point);
  return time <= kNullTime ? kNullTime : time;
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

namespace {

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

}  // namespace

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

namespace {

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

}  // namespace

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

LocalizedText ReadLocalizedTexts(pugi::xml_node node, const char* name) {
  std::vector<LocalizedText> translations;
  for (pugi::xml_node child : node.children(name)) {
    translations.emplace_back(
        child.attribute("Locale").as_string(),
        UtfConvert<char16_t>(std::string_view{child.text().as_string()}));
  }
  return EncodeMultiLanguage(translations);
}

void AppendLocalizedTexts(pugi::xml_node node,
                          const char* name,
                          const LocalizedText& text) {
  // A packed "mul" value carries every translation; UANodeSet lists them as
  // repeated elements, one per Locale (Part 6 §F.3).
  for (const LocalizedText& translation : DecodeMultiLanguage(text)) {
    pugi::xml_node element = node.append_child(name);
    if (!translation.locale.empty()) {
      element.append_attribute("Locale").set_value(translation.locale.c_str());
    }
    element.text().set(UtfConvert<char>(translation.text).c_str());
  }
}

}  // namespace scada::uanodeset
