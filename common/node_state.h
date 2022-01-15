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
  NodeClass node_class = NodeClass::Object;
  NodeId type_definition_id;
  NodeId parent_id;
  NodeId reference_type_id;
  NodeAttributes attributes;
  NodeProperties properties;
  std::vector<ReferenceDescription> references;
  std::vector<NodeState> children;
  NodeId supertype_id;

  NodeState& set_node_id(NodeId node_id) {
    this->node_id = std::move(node_id);
    return *this;
  }

  NodeState& set_node_class(NodeClass node_class) {
    this->node_class = node_class;
    return *this;
  }

  NodeState& set_type_definition_id(NodeId type_definition_id) {
    this->type_definition_id = std::move(type_definition_id);
    return *this;
  }

  NodeState& set_parent(NodeId reference_type_id, NodeId parent_id) {
    this->reference_type_id = std::move(reference_type_id);
    this->parent_id = std::move(parent_id);
    return *this;
  }

  NodeState& set_attributes(const NodeAttributes& attributes) {
    this->attributes = attributes;
    return *this;
  }

  NodeState& set_display_name(const LocalizedText& display_name) {
    attributes.display_name = display_name;
    return *this;
  }

  NodeState& set_supertype_id(NodeId supertype_id) {
    this->supertype_id = std::move(supertype_id);
    return *this;
  }
};

inline const NodeId* FindReference(
    const std::vector<ReferenceDescription>& references,
    const NodeId& reference_type_id,
    bool forward) {
  auto i = std::find_if(references.begin(), references.end(), [&](auto& p) {
    return p.forward == forward && p.reference_type_id == reference_type_id;
  });
  return i != references.end() ? &i->node_id : nullptr;
}

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
