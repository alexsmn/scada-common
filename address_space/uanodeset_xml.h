#pragma once

#include "base/time/time.h"
#include "scada/basic_types.h"
#include "scada/localized_text.h"
#include "scada/node_id.h"
#include "scada/qualified_name.h"
#include "scada/variant.h"

#include <pugixml.hpp>

#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// The pieces of the OPC UA UANodeSet XML format (Part 6 Annex F,
// https://reference.opcfoundation.org/Core/Part6/v105/docs/F.1) and of the UA
// XML value encoding it embeds (Part 6 §5.3,
// https://reference.opcfoundation.org/Core/Part6/v105/docs/5.3) that every
// reader and writer of the format here shares: the static model loader
// (address_space_xml.cpp) and the configuration export/import codec
// (configuration_nodeset.cpp). What a document's nodes MEAN differs between
// the two; how its ids, names, times and values are spelled does not.
namespace scada::uanodeset {

// XML namespaces of the UANodeSet schema and of the UA XML value types.
inline constexpr char kNodeSetXmlns[] =
    "http://opcfoundation.org/UA/2011/03/UANodeSet.xsd";
inline constexpr char kTypesXmlns[] =
    "http://opcfoundation.org/UA/2008/02/Types.xsd";
inline constexpr std::string_view kOpcUaNamespaceUri =
    "http://opcfoundation.org/UA/";

// Maps the in-process namespace index to and from a document's own: its
// NamespaceUris table, where local index 0 is always the OPC UA namespace and
// local index i is the i-th Uri (Part 6 §F.2).
class NamespaceTable {
 public:
  // `uris[i]` is the URI of in-process index i. A document URI that is not
  // among them resolves to `fallback` when one is given, and fails otherwise.
  explicit NamespaceTable(std::span<const std::string> uris,
                          std::optional<NamespaceIndex> fallback = {})
      : uris_{uris}, fallback_{fallback} {}

  // Writing: registers `index` as used.
  void Use(NamespaceIndex index);

  // Writing: fixes the document's local indexes, ascending by in-process
  // index so the output is deterministic. False when an index has no URI.
  bool Assign();

  // Writing: the local index of in-process `index`. Requires Assign().
  NamespaceIndex ToLocal(NamespaceIndex index) const;

  const std::set<NamespaceIndex>& used() const { return used_; }

  // Reading: appends the document's next Uri. False when it is unknown and
  // there is no fallback.
  bool AddLocal(std::string_view uri);

  // Reading: the in-process index of document index `local`.
  std::optional<NamespaceIndex> FromLocal(NamespaceIndex local) const;

  // Reading: every Uri of `root`'s NamespaceUris, in order. False as AddLocal.
  bool AddLocals(pugi::xml_node root);

 private:
  std::span<const std::string> uris_;
  std::optional<NamespaceIndex> fallback_;
  std::set<NamespaceIndex> used_;
  std::map<NamespaceIndex, NamespaceIndex> to_local_;
  std::vector<NamespaceIndex> from_local_;
};

// A document's Aliases table: alias name -> NodeId text.
using AliasMap = std::unordered_map<std::string, std::string>;

// Reads `root`'s Aliases.
AliasMap ReadAliases(pugi::xml_node root);

// The Part 6 §5.1.12 string form of `node_id`, with the document's namespace
// index.
std::string LocalNodeIdText(const NodeId& node_id,
                            const NamespaceTable& namespaces);

// "<local ns>:<name>", or just the name in namespace 0 (Part 6 §F.3).
std::string LocalQualifiedNameText(const QualifiedName& name,
                                   const NamespaceTable& namespaces);

// Parses a NodeId written in the document — an alias or the §5.1.12 form —
// into the in-process namespace; nullopt when it does not parse or names a
// namespace the document does not declare.
std::optional<NodeId> ParseNodeIdText(std::string_view text,
                                      const NamespaceTable& namespaces,
                                      const AliasMap& aliases);

// Parses a QualifiedName written "<local ns>:<name>" or "<name>".
std::optional<QualifiedName> ParseQualifiedNameText(
    std::string_view text,
    const NamespaceTable& namespaces);

// A DateTime as xs:dateTime (Part 6 §5.3.1.6,
// https://reference.opcfoundation.org/Core/Part6/v105/docs/5.3.1.6): UTC, with
// the seven fractional digits of the OPC UA 100 ns resolution. The earliest
// and latest values "shall not be literally encoded": the null time is
// written 0001-01-01T00:00:00Z and kMaxTime 9999-12-31T23:59:59Z.
std::string TimeText(Time time);

// Parses an xs:dateTime in UTC ("Z") or with an explicit "+HH:MM"/"-HH:MM"
// offset, which §5.3.1.6 also allows. Anything at or before the 1601 epoch —
// the null DateTime's written form among them — reads as kNullTime, and
// 9999-12-31T23:59:59Z or later as kMaxTime: "The XML decoder should not
// generate an error if it encounters an out of range date value."
std::optional<Time> ParseTimeText(std::string_view text);

// The UA XML element name of `value`'s built-in type (Part 6 §5.3.1), or
// nullopt when the format as used here does not carry it (arrays and
// ExtensionObjects among them).
std::optional<std::string_view> TypeElementName(const Variant& value);

// Appends `value` to `parent` as a uax: element (Part 6 §5.3); false when
// unsupported. The document root must declare the uax prefix as kTypesXmlns.
bool WriteValue(pugi::xml_node parent,
                const Variant& value,
                const NamespaceTable& namespaces);

// Reads the uax: element inside `value_node` (a <Value>); nullopt when it
// holds no element, an unsupported type or text that does not parse.
std::optional<Variant> ReadValue(pugi::xml_node value_node,
                                 const NamespaceTable& namespaces,
                                 const AliasMap& aliases);

// Reads every `name` child of `node` (DisplayName, Description, InverseName),
// each with an optional Locale, packed into one multi-language value.
//
// The elements are `maxOccurs="unbounded"` in the NodeSet schema and their
// type extends `xs:string` with a `Locale` attribute defaulting to ""
// (UANodeSet.xsd, `UANode` and `LocalizedText`,
// https://raw.githubusercontent.com/OPCFoundation/UA-Nodeset/latest/Schema/UANodeSet.xsd,
// read 2026-09-20), so a translated node is written as siblings:
//
//     <DisplayName Locale="ru">Все объекты</DisplayName>
//     <DisplayName Locale="en">All objects</DisplayName>
//
// More than one gives the packed "mul" form, which the client-facing service
// boundary resolves per session (OPC UA Part 4 §5.4,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/5.4). One gives
// that element unchanged, so an untranslated node is exactly what it was
// before locale-qualified names existed. An element with no Locale keeps an
// empty locale rather than a guessed one: it cannot be asked for by name,
// but it stays first and so is what §5.4's "return an available locale"
// falls back to.
LocalizedText ReadLocalizedTexts(pugi::xml_node node, const char* name);

// Appends `text` as one `name` child per language.
void AppendLocalizedTexts(pugi::xml_node node,
                          const char* name,
                          const LocalizedText& text);

// The element name without its prefix.
std::string_view LocalName(std::string_view qualified);

// The first element child of `parent`, skipping text and comments.
pugi::xml_node FirstElement(pugi::xml_node parent);

// The first element child of `parent` whose local name is `name`.
pugi::xml_node ChildByLocalName(pugi::xml_node parent, std::string_view name);

}  // namespace scada::uanodeset
