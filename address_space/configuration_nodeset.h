#pragma once

#include "common/node_state.h"
#include "scada/basic_types.h"
#include "scada/qualified_name.h"
#include "scada/status_or.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// Configuration as an OPC UA UANodeSet document (OPC UA Part 6 Annex F,
// https://reference.opcfoundation.org/Core/Part6/v105/docs/F.2) — the file
// format of ADR 0014's configuration export and import. See
// docs/adr/0014-configuration-transfer.md.
//
// A configuration node is written as a UAObject or UAVariable with its
// DisplayName in every language it has, its parent as ParentNodeId plus the
// inverse hierarchical Reference, its type as HasTypeDefinition, and its other
// References as they are. Its properties — which a NodeState carries as
// (declaration id, value) pairs — become child UAVariables of PropertyType
// under HasProperty, named by the declaration's BrowseName and given the
// nested NodeId `<parent>!<BrowseName>` the address space already uses for
// them. Values use the UA XML encoding (Part 6 §5.3).
//
// Namespaces are written by URI (the document's NamespaceUris table), never by
// the process's in-memory index, so a file moves between servers whose
// NamespaceArrays differ (Part 6 §F.2, §F.14).
namespace scada {

// Maps a node's properties to and from the BrowseNames a NodeSet names them
// by. The configuration's table schemas know both directions.
struct NodeSetPropertyNames {
  // The BrowseName of property `prop_decl_id` on a node of type
  // `type_definition_id`, or nullopt when the type has no such property.
  std::function<std::optional<QualifiedName>(const NodeId& type_definition_id,
                                             const NodeId& prop_decl_id)>
      browse_name;
  // The property declaration a child named `browse_name` refers to on a node
  // of type `type_definition_id`, or a null NodeId when there is none.
  std::function<NodeId(const NodeId& type_definition_id,
                       const QualifiedName& browse_name)>
      declaration;
};

// A configuration NodeSet: the nodes, and what the vendor Extensions element
// records about the export they came from.
struct ConfigurationNodeSet {
  std::vector<NodeState> nodes;
  // Identifies the configuration the file was exported from, so an import
  // can tell whether the configuration has changed since. Opaque here.
  std::string version;
  // The namespace URIs the export covered — what a full replace replaces.
  std::vector<std::string> scope;
  // The document's LastModified: when it was written.
  Time last_modified;
};

// Writes `nodeset` as a UANodeSet document. `namespace_uris[i]` is the URI of
// in-process namespace index i (index 0, the OPC UA namespace, is implicit).
// Nodes are written sorted by NodeId, so equal input gives byte-equal output.
// Fails with Bad_NotSupported for a node class or value it cannot represent
// (anything but an Object or Variable; arrays; ExtensionObjects) and with
// Bad_WrongPropertyId for a property `names` cannot name.
StatusOr<std::string> WriteConfigurationNodeSet(
    const ConfigurationNodeSet& nodeset,
    std::span<const std::string> namespace_uris,
    const NodeSetPropertyNames& names);

// Reads a document written by WriteConfigurationNodeSet, or any UANodeSet
// using the same subset. Fails with Bad_CantParseString for malformed XML or
// a missing UANodeSet root, Bad_WrongNodeId for a NodeId naming a namespace
// URI absent from `namespace_uris`, and Bad_WrongPropertyId for a property
// child `names` cannot resolve.
StatusOr<ConfigurationNodeSet> ReadConfigurationNodeSet(
    std::string_view xml,
    std::span<const std::string> namespace_uris,
    const NodeSetPropertyNames& names);

// --- Changes: UANodeSetChanges and UANodeSetChangesStatus -------------------
//
// The diff format (Part 6 §F.16,
// https://reference.opcfoundation.org/Core/Part6/v105/docs/F.16) and its
// result (§F.22,
// https://reference.opcfoundation.org/Core/Part6/v105/docs/F.22). Element and
// attribute names follow the published schema, UANodeSet.xsd
// (https://raw.githubusercontent.com/OPCFoundation/UA-Nodeset/4194ae486d36166d6997de9946d361ec2ce4a4fe/Schema/UANodeSet.xsd,
// verified 2026-09-26), where it differs from Part 6's prose tables: a
// ReferenceToChange's Target is its element text and Source, ReferenceType
// and IsForward its attributes; TransactionId is an attribute of both roots;
// a status list's entries are `Status` elements carrying `Code` as an
// attribute and Details as text; and the status root has no NamespaceUris.

// One entry of ReferencesToAdd or ReferencesToDelete (§F.19).
struct NodeSetReferenceChange {
  NodeId source;
  NodeId reference_type_id;
  bool forward = true;
  NodeId target;

  bool operator==(const NodeSetReferenceChange&) const = default;
};

// One entry of NodesToDelete (§F.21). DeleteReverseReferences defaults to
// true in the schema.
struct NodeSetNodeDeletion {
  NodeId node_id;
  bool delete_reverse_references = true;

  bool operator==(const NodeSetNodeDeletion&) const = default;
};

// One NodeSetStatus (§F.24): the OPC UA status code as it goes on the wire
// (Part 4 §7.38.2), and details that are "not a human readable string for the
// StatusCode".
struct NodeSetOperationStatus {
  std::uint32_t code = 0;
  std::string details;

  bool operator==(const NodeSetOperationStatus&) const = default;
};

// A UANodeSetChangesStatus (§F.22). Each list is empty when every operation
// of its kind succeeded, and otherwise holds one entry per operation (§F.23).
struct NodeSetChangesStatus {
  std::string transaction_id;
  Time last_modified;
  std::vector<NodeSetOperationStatus> nodes_to_add;
  std::vector<NodeSetOperationStatus> references_to_add;
  std::vector<NodeSetOperationStatus> nodes_to_delete;
  std::vector<NodeSetOperationStatus> references_to_delete;
};

// What a configuration import did with a change set — the vendor extension a
// result document carries beside its UANodeSetChangesStatus.
struct ConfigurationImportOutcome {
  // Whether the changes were written. False for a dry run and for a set
  // that failed; either way nothing was.
  bool committed = false;
  bool dry_run = false;
  // The configuration's version after the import, for the next one to name
  // as its base. Empty when nothing was committed.
  std::string version;
  NodeSetChangesStatus status;
};

// A configuration UANodeSetChanges document. Nodes to add are written as in
// a UANodeSet; a property child whose owner is not itself added stays a node
// of its own — a PropertyType UAVariable under HasProperty — which is how a
// change to one property of an existing node is spelled.
struct ConfigurationNodeSetChanges {
  std::string transaction_id;
  Time last_modified;
  // The configuration version the changes were made against and the scope
  // it was taken over — the namespace URIs of the export the changes were
  // made from — carried in the same ConfigurationExport extension an export
  // writes. An empty version asks for no check.
  std::string version;
  std::vector<std::string> scope;
  std::vector<NodeState> nodes_to_add;
  std::vector<NodeSetReferenceChange> references_to_add;
  std::vector<NodeSetNodeDeletion> nodes_to_delete;
  std::vector<NodeSetReferenceChange> references_to_delete;
  // Set on an import's result document: what happened to these changes.
  std::optional<ConfigurationImportOutcome> outcome;
};

// Writes `changes` as a UANodeSetChanges document, AcceptAllOrNothing — every
// change set this format carries is applied whole. Fails as
// WriteConfigurationNodeSet does.
StatusOr<std::string> WriteConfigurationNodeSetChanges(
    const ConfigurationNodeSetChanges& changes,
    std::span<const std::string> namespace_uris,
    const NodeSetPropertyNames& names);

// Reads a UANodeSetChanges document. Fails as ReadConfigurationNodeSet does,
// and with Bad_CantParseString when the root is not UANodeSetChanges.
StatusOr<ConfigurationNodeSetChanges> ReadConfigurationNodeSetChanges(
    std::string_view xml,
    std::span<const std::string> namespace_uris,
    const NodeSetPropertyNames& names);

// The root element's local name — "UANodeSet", "UANodeSetChanges" — or empty
// when `xml` does not parse. What an import decides its operation by.
std::string NodeSetDocumentKind(std::string_view xml);

}  // namespace scada
