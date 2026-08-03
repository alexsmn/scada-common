#pragma once

#include "scada/node_id.h"

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
