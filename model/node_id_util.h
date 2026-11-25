#pragma once

#include "scada/node_id.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace scada {

// Normal nodes are not considered nested.
//
// WARNING: This method must be very performant, as a service locator may invoke
// it for each requested node.
//
// TODO: Return `std::optional<std::string_view>` instead of `bool`.
bool IsNestedNodeId(const scada::NodeId& node_id,
                    scada::NodeId& parent_id,
                    std::string_view& nested_name);

inline bool IsNestedNodeId(const scada::NodeId& node_id) {
  scada::NodeId parent_id;
  std::string_view nested_name;
  return IsNestedNodeId(node_id, parent_id, nested_name);
}

bool GetRootNestedNodeId(const scada::NodeId& node_id,
                         scada::NodeId& parent_id,
                         std::string_view& nested_name);

// `nested_name` must not be empty.
scada::NodeId MakeNestedNodeId(const scada::NodeId& parent_id,
                               std::string_view nested_name);

// `nested_name` must not be empty.
scada::NodeId MakeNestedNodeId(const scada::NodeId& parent_id,
                               std::string_view nested_name,
                               scada::NamespaceIndex namespace_index);

bool GetNestedSubName(const scada::NodeId& node_id,
                      const scada::NodeId& nested_id,
                      std::string_view& nested_name);

scada::NodeId NodeIdFromScadaString(std::string_view scada_string);
std::string NodeIdToScadaString(const scada::NodeId& node_id);

// A connection's index for a namespace URI, or nullopt when it has none
// (DataServices::namespace_index_for_uri_).
using NamespaceIndexForUri =
    std::function<std::optional<scada::NamespaceIndex>(std::string_view uri)>;

// Reads a stored node reference in the OPC UA Part 6 §5.1.12 string form
// (scada/node_id_string.h): `nsu=<uri>;<identifier>`, or a bare identifier
// for namespace 0 (`i=2258`). Returns nullopt when `text` is not in that
// form, so the caller tries its other forms (an alias, `TS.1379`). Returns a
// null NodeId when it is, but names nothing this connection can resolve: a
// URI neither `index_for_uri` nor the model's canonical table knows, or the
// `ns=<n>;` form, whose index belongs to the session that wrote it.
// `index_for_uri` may be empty; the canonical table is then the only one.
std::optional<scada::NodeId> NodeIdFromNodeIdString(
    std::string_view text,
    const NamespaceIndexForUri& index_for_uri);

// Installs the model's namespace-name table into `scada_core` so that
// `scada::NodeIdToLogString` (used by lower layers such as the remote protocol,
// which cannot depend on the model) renders friendly namespace prefixes like
// "SCADA.123" / "USER.5". Call once at startup, next to
// `scada::RegisterPathProvider()`; mirrors that provider-registration pattern.
// Without it, those logs fall back to the numeric "NS<n>" prefix.
void RegisterModelNamespaceResolver();

}  // namespace scada

// Transitional compatibility shim: these node-id utilities historically lived
// at global scope. Expose them there until all callers migrate to `scada::`.
using scada::GetNestedSubName;       // NOLINT(build/namespaces) transitional
using scada::GetRootNestedNodeId;    // NOLINT(build/namespaces) transitional
using scada::IsNestedNodeId;         // NOLINT(build/namespaces) transitional
using scada::MakeNestedNodeId;       // NOLINT(build/namespaces) transitional
using scada::NodeIdFromScadaString;  // NOLINT(build/namespaces) transitional
using scada::NodeIdToScadaString;    // NOLINT(build/namespaces) transitional
