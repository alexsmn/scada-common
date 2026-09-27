#include "address_space/configuration_nodeset.h"

#include "address_space/uanodeset_xml.h"

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

using namespace uanodeset;

// The vendor namespace of this format's Extensions element (Part 6 §F.2:
// "free form XML data that can be used to attach vendor defined data").
constexpr char kExportXmlns[] =
    "http://telecontrol.ru/opcua/configuration-export";

// Standard namespace-0 NodeIds this format names.
constexpr NumericId kOrganizes = 35;
constexpr NumericId kHasTypeDefinition = 40;
constexpr NumericId kHasProperty = 46;
constexpr NumericId kPropertyType = 68;

bool IsStandard(const NodeId& node_id, NumericId numeric_id) {
  return node_id.namespace_index() == 0 && node_id.is_numeric() &&
         node_id.numeric_id() == numeric_id;
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
  AppendLocalizedTexts(element, "DisplayName", node.attributes.display_name);

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

  state.attributes.display_name = ReadLocalizedTexts(element, "DisplayName");

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

// How AppendNodes orders the nodes it writes.
enum class NodeOrder {
  // By NodeId, so equal input gives byte-equal output — what lets a hash of a
  // UANodeSet identify a configuration.
  kSorted,
  // As given. A UANodeSetChanges' NodesToAdd is a list of operations, and its
  // status list (Part 6 §F.23) holds one entry per operation in the same
  // order, so reordering the nodes would report each outcome against another
  // node.
  kAsGiven,
};

// Appends `nodes` to `container` in `order`.
Status AppendNodes(pugi::xml_node container,
                   std::span<const NodeState> nodes,
                   const NamespaceTable& namespaces,
                   const NodeSetPropertyNames& names,
                   NodeOrder order) {
  std::vector<const NodeState*> sorted;
  sorted.reserve(nodes.size());
  for (const NodeState& node : nodes) {
    sorted.push_back(&node);
  }
  if (order == NodeOrder::kSorted) {
    std::ranges::sort(sorted, [](const NodeState* a, const NodeState* b) {
      return NodeIdLess(a->node_id, b->node_id);
    });
  }
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
  DocumentContext context{NamespaceTable{namespace_uris}, ReadAliases(root)};
  if (!context.namespaces.AddLocals(root)) {
    return StatusCode::Bad_WrongNodeId;
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

  if (auto status = AppendNodes(root, nodeset.nodes, namespaces, names,
                                NodeOrder::kSorted);
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
    if (auto status =
            AppendNodes(root.append_child("NodesToAdd"), changes.nodes_to_add,
                        namespaces, names, NodeOrder::kAsGiven);
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

namespace {

bool IsPropertyChildElement(pugi::xml_node element) {
  for (pugi::xml_node reference :
       element.child("References").children("Reference")) {
    if (std::string_view{reference.attribute("ReferenceType").as_string()} ==
            "i=40" &&
        reference.attribute("IsForward").as_bool(true) &&
        std::string_view{reference.text().as_string()} == "i=68") {
      return true;
    }
  }
  return false;
}

// The operations of a NodesToAdd list, by NodeId text.
std::vector<std::string> AddedNodeIds(pugi::xml_node list) {
  std::set<std::string> added;
  for (pugi::xml_node element : list.children()) {
    if (element.type() == pugi::node_element) {
      added.insert(element.attribute("NodeId").as_string());
    }
  }
  std::vector<std::string> operations;
  for (pugi::xml_node element : list.children()) {
    if (element.type() != pugi::node_element) {
      continue;
    }
    if (IsPropertyChildElement(element) &&
        added.contains(element.attribute("ParentNodeId").as_string())) {
      continue;
    }
    operations.emplace_back(element.attribute("NodeId").as_string());
  }
  return operations;
}

std::vector<std::string> ReferenceSources(pugi::xml_node list) {
  std::vector<std::string> sources;
  for (pugi::xml_node reference : list.children("Reference")) {
    sources.emplace_back(reference.attribute("Source").as_string());
  }
  return sources;
}

}  // namespace

StatusOr<ImportResultSummary> ReadImportResultSummary(std::string_view xml) {
  pugi::xml_document document;
  if (!document.load_buffer(xml.data(), xml.size())) {
    return StatusCode::Bad_CantParseString;
  }
  const pugi::xml_node root = document.child("UANodeSetChanges");
  if (!root) {
    return StatusCode::Bad_CantParseString;
  }

  ImportResultSummary summary;
  for (pugi::xml_node import_info :
       FindExtensions(root, "ConfigurationImport")) {
    summary.committed = import_info.attribute("Committed").as_bool();
    summary.dry_run = import_info.attribute("DryRun").as_bool();
    summary.version = import_info.attribute("Version").as_string();
  }

  const std::vector<std::string> nodes_to_add =
      AddedNodeIds(root.child("NodesToAdd"));
  std::vector<std::string> nodes_to_delete;
  for (pugi::xml_node node : root.child("NodesToDelete").children("Node")) {
    nodes_to_delete.emplace_back(node.text().as_string());
  }
  const std::vector<std::string> references_to_add =
      ReferenceSources(root.child("ReferencesToAdd"));
  const std::vector<std::string> references_to_delete =
      ReferenceSources(root.child("ReferencesToDelete"));

  const std::set<std::string> deleted{nodes_to_delete.begin(),
                                      nodes_to_delete.end()};
  const std::set<std::string> added{nodes_to_add.begin(), nodes_to_add.end()};
  for (const std::string& node_id : nodes_to_add) {
    (deleted.contains(node_id) ? summary.modified : summary.added)
        .push_back(node_id);
  }
  for (const std::string& node_id : nodes_to_delete) {
    if (!added.contains(node_id)) {
      summary.deleted.push_back(node_id);
    }
  }
  summary.references_added = references_to_add.size();
  summary.references_deleted = references_to_delete.size();

  for (pugi::xml_node status_root :
       FindExtensions(root, "UANodeSetChangesStatus")) {
    const NodeSetChangesStatus status = ReadChangesStatus(status_root);
    const auto collect = [&](std::string_view list,
                             const std::vector<NodeSetOperationStatus>& entries,
                             const std::vector<std::string>& targets) {
      for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].code != 0) {
          summary.failures.push_back(
              {.list = std::string{list},
               .target = i < targets.size() ? targets[i] : std::string{},
               .status = entries[i]});
        }
      }
    };
    collect("NodesToAdd", status.nodes_to_add, nodes_to_add);
    collect("ReferencesToAdd", status.references_to_add, references_to_add);
    collect("NodesToDelete", status.nodes_to_delete, nodes_to_delete);
    collect("ReferencesToDelete", status.references_to_delete,
            references_to_delete);
  }
  return summary;
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
