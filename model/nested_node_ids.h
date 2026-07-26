#pragma once

#include "model/node_id_util.h"

#include <optional>

namespace scada {

struct NestedNodeId {
  scada::NodeId parent_id;
  std::string_view nested_name;
};

// BrowseName of the Control object a commandable data item carries, and so the
// nested-name component of its node id (`<item>!Control`). Single source for
// the three places that must agree on the spelling: the BrowseName in
// data_items.xml, the server-side locator that routes Select/Operate/Cancel to
// it, and the client that addresses it.
inline constexpr std::string_view kControlObjectName = "Control";

inline std::optional<NestedNodeId> ParseNestedNodeId(
    const scada::NodeId& node_id) {
  scada::NodeId parent_id;
  std::string_view nested_name;
  return IsNestedNodeId(node_id, parent_id, nested_name)
             ? std::optional<NestedNodeId>{NestedNodeId{parent_id, nested_name}}
             : std::optional<NestedNodeId>{};
}

}  // namespace scada

// Transitional compatibility shim: expose the historically global-scope names
// until all callers migrate to `scada::`.
using scada::NestedNodeId;       // NOLINT(build/namespaces) transitional
using scada::ParseNestedNodeId;  // NOLINT(build/namespaces) transitional