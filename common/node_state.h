#pragma once

#include "base/debug_util.h"
#include "core/node_attributes.h"
#include "core/node_class.h"
#include "core/view_service.h"

#include <ostream>

namespace scada {

struct ReferenceState {
  NodeId reference_type_id;
  NodeId source_id;
  NodeId target_id;
};

struct NodeState {
  NodeId node_id;
  NodeClass node_class = scada::NodeClass::Object;
  NodeId type_definition_id;
  NodeId parent_id;
  NodeId reference_type_id;
  NodeAttributes attributes;
  NodeProperties properties;
  std::vector<ReferenceDescription> references;
  std::vector<NodeState> children;
  NodeId supertype_id;

  NodeState& set_attributes(const NodeAttributes& attributes) {
    this->attributes = attributes;
    return *this;
  }

  NodeState& set_display_name(const LocalizedText& display_name) {
    attributes.display_name = display_name;
    return *this;
  }
};

inline bool operator==(const ReferenceState& a, const ReferenceState& b) {
  return a.reference_type_id == b.reference_type_id &&
         a.source_id == b.source_id && a.target_id == b.target_id;
}

inline std::ostream& operator<<(std::ostream& stream,
                                const NodeState& node_state) {
  using ::operator<<;
  return stream << "{"
                << "node_id: " << node_state.node_id << ", "
                << "node_class: " << node_state.node_class << ", "
                << "type_definition_id: " << node_state.type_definition_id
                << ", "
                << "parent_id: " << node_state.parent_id << ", "
                << "reference_type_id: " << node_state.reference_type_id << ", "
                << "supertype_id: " << node_state.supertype_id << ", "
                << "attributes: " << node_state.attributes << ", "
                << "properties: " << node_state.properties << ", "
                << "references: " << node_state.references << ", "
                << "children: " << node_state.children << "}";
}

}  // namespace scada
