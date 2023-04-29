#pragma once

#include "base/debug_util.h"
#include "core/node_attributes.h"
#include "core/node_class.h"
#include "core/node_id.h"
#include "core/variant.h"
#include "core/view_service.h"

#include <memory>
#include <ostream>
#include <vector>

namespace scada {

struct ReferenceState {
  NodeId reference_type_id;
  NodeId source_id;
  NodeId target_id;
};

using NodeProperty = std::pair<NodeId /*prop_type_id*/, Variant /*value*/>;
using NodeProperties = std::vector<NodeProperty>;

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

  NodeState& set_browse_name(const QualifiedName& browse_name) {
    attributes.browse_name = browse_name;
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

  NodeState& set_properties(NodeProperties properties) {
    this->properties = std::move(properties);
    return *this;
  }

  NodeState& set_property(const scada::NodeId& prop_decl_id,
                          scada::Variant value);

  NodeState& add_reference(scada::ReferenceDescription ref) {
    this->references.emplace_back(std::move(ref));
    return *this;
  }

  WARN_UNUSED_RESULT std::optional<scada::Variant> GetAttribute(
      AttributeId attribute_id) const;
};

using NodeStatePtr = std::shared_ptr<const NodeState>;

inline std::optional<scada::Variant> NodeState::GetAttribute(
    AttributeId attribute_id) const {
  switch (attribute_id) {
    case AttributeId::NodeId:
      return node_id;
    case AttributeId::NodeClass:
      return static_cast<scada::Int32>(node_class);
    default:
      return attributes.Get(attribute_id);
  }
}

inline WARN_UNUSED_RESULT const ReferenceDescription* FindReference(
    const std::vector<ReferenceDescription>& references,
    const NodeId& reference_type_id,
    bool forward) {
  auto i = std::ranges::find_if(references, [&](auto& p) {
    return p.forward == forward && p.reference_type_id == reference_type_id;
  });
  return i != references.end() ? std::to_address(i) : nullptr;
}

inline WARN_UNUSED_RESULT const NodeId* FindReferenceTarget(
    const std::vector<ReferenceDescription>& references,
    const NodeId& reference_type_id,
    bool forward) {
  auto* ref = FindReference(references, reference_type_id, forward);
  return ref ? &ref->node_id : nullptr;
}

inline WARN_UNUSED_RESULT Variant* FindProperty(NodeProperties& properties,
                                                const NodeId& prop_decl_id) {
  auto i = std::ranges::find(properties, prop_decl_id,
                             [&](auto& p) { return p.first; });
  return i != properties.end() ? &i->second : nullptr;
}

inline WARN_UNUSED_RESULT const Variant* FindProperty(
    const NodeProperties& properties,
    const NodeId& prop_decl_id) {
  return FindProperty(const_cast<NodeProperties&>(properties), prop_decl_id);
}

inline WARN_UNUSED_RESULT Variant GetProperty(const NodeProperties& properties,
                                              const NodeId& prop_decl_id) {
  auto* p = FindProperty(properties, prop_decl_id);
  return p ? *p : Variant{};
}

// TODO: Rename to `UpdateProperty` or `ReplaceProperty`.
inline WARN_UNUSED_RESULT bool SetProperty(NodeProperties& properties,
                                           const NodeId& prop_decl_id,
                                           scada::Variant value) {
  auto* prop = FindProperty(properties, prop_decl_id);
  if (!prop)
    return false;
  *prop = std::move(value);
  return true;
}

// TODO: Rename to `SetProperty` for consistency with `SetReference`.
inline void SetOrAddOrDeleteProperty(NodeProperties& properties,
                                     const NodeId& prop_decl_id,
                                     scada::Variant value) {
  auto i = std::ranges::find(properties, prop_decl_id,
                             [&](auto& p) { return p.first; });
  if (i != properties.end()) {
    if (value.is_null()) {
      properties.erase(i);
    } else {
      i->second = std::move(value);
    }
  } else if (!value.is_null()) {
    properties.emplace_back(prop_decl_id, std::move(value));
  }
}

inline void SetProperties(NodeProperties& properties,
                          base::span<const NodeProperty> updated_properties) {
  for (auto& [prop_decl_id, prop_value] : updated_properties) {
    SetOrAddOrDeleteProperty(properties, prop_decl_id, std::move(prop_value));
  }
}

inline WARN_UNUSED_RESULT scada::ReferenceDescription* FindReference(
    scada::ReferenceDescriptions& references,
    const scada::NodeId& reference_type_id,
    bool forward) {
  auto i = std::ranges::find_if(references, [&](auto&& ref) {
    return ref.reference_type_id == reference_type_id && ref.forward == forward;
  });
  return i != references.end() ? std::to_address(i) : nullptr;
}

inline void SetReference(scada::ReferenceDescriptions& references,
                         const scada::ReferenceDescription& updated_reference) {
  auto i = std::ranges::find_if(references, [&](const auto& ref) {
    return ref.forward == updated_reference.forward &&
           ref.reference_type_id == updated_reference.reference_type_id;
  });
  if (i != references.end()) {
    if (updated_reference.node_id.is_null()) {
      references.erase(i);
    } else {
      i->node_id = updated_reference.node_id;
    }
  } else if (!updated_reference.node_id.is_null()) {
    references.emplace_back(updated_reference);
  }
}

inline void SetReferences(
    scada::ReferenceDescriptions& references,
    base::span<const ReferenceDescription> updated_references) {
  for (auto& ref : updated_references)
    SetReference(references, ref);
}

inline NodeState& NodeState::set_property(const scada::NodeId& prop_decl_id,
                                          scada::Variant value) {
  SetOrAddOrDeleteProperty(properties, prop_decl_id, std::move(value));
  return *this;
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
