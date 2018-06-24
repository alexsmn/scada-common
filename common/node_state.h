#pragma once

#include "core/configuration_types.h"
#include "core/node_attributes.h"
#include "core/node_class.h"

namespace scada {

struct ReferenceState {
  NodeId reference_type_id;
  NodeId source_id;
  NodeId target_id;
};

struct NodeState {
  NodeId node_id;
  NodeClass node_class;
  NodeId type_definition_id;
  NodeId parent_id;
  NodeId reference_type_id;
  NodeAttributes attributes;
  NodeProperties properties;
  std::vector<ReferenceDescription> references;
  NodeId super_type_id;
  std::vector<NodeState> children;
};

inline bool operator==(const ReferenceState& a, const ReferenceState& b) {
  return a.reference_type_id == b.reference_type_id &&
         a.source_id == b.source_id && a.target_id == b.target_id;
}

inline std::ostream& operator<<(std::ostream& stream,
                                const NodeState& node_state) {
  return stream << "{" << node_state.node_id << ", " << node_state.node_class
                << ", " << node_state.type_definition_id << ", "
                << node_state.parent_id << ", " << node_state.reference_type_id
                << ", " << node_state.attributes << ", "
                << node_state.properties << ", " << node_state.references
                << "}";
}

}  // namespace scada
