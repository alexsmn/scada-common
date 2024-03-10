#pragma once

#include "scada/node_attributes.h"
#include "scada/node_class.h"
#include "scada/node_id.h"
#include "scada/variant.h"
#include "scada/view_service.h"

#include <memory>
#include <ostream>
#include <vector>

namespace scada {

struct ReferenceState {
  bool operator==(const ReferenceState&) const = default;

  NodeId reference_type_id;
  NodeId source_id;
  NodeId target_id;
};

using NodeProperty = std::pair<NodeId /*prop_type_id*/, Variant /*value*/>;
using NodeProperties = std::vector<NodeProperty>;

// WARNING: Don't declare equality operator, since order of containers is
// important. Use `common/test/node_state_matcher.h` instead.
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

  [[nodiscard]] std::optional<scada::Variant> GetAttribute(
      AttributeId attribute_id) const;

  bool HasReference(const scada::ReferenceDescription& desc) const {
    return std::ranges::find(references, desc) != references.end();
  }
};

using NodeStatePtr = std::shared_ptr<const NodeState>;

[[nodiscard]] const ReferenceDescription* FindReference(
    const std::vector<ReferenceDescription>& references,
    const NodeId& reference_type_id,
    bool forward);

inline [[nodiscard]] const NodeId* FindReferenceTarget(
    const std::vector<ReferenceDescription>& references,
    const NodeId& reference_type_id,
    bool forward) {
  auto* ref = FindReference(references, reference_type_id, forward);
  return ref ? &ref->node_id : nullptr;
}

[[nodiscard]] Variant* FindProperty(NodeProperties& properties,
                                    const NodeId& prop_decl_id);

inline [[nodiscard]] const Variant* FindProperty(
    const NodeProperties& properties,
    const NodeId& prop_decl_id) {
  return FindProperty(const_cast<NodeProperties&>(properties), prop_decl_id);
}

inline [[nodiscard]] Variant GetProperty(const NodeProperties& properties,
                                         const NodeId& prop_decl_id) {
  auto* p = FindProperty(properties, prop_decl_id);
  return p ? *p : Variant{};
}

// TODO: Rename to `UpdateProperty` or `ReplaceProperty`.
[[nodiscard]] bool SetProperty(NodeProperties& properties,
                               const NodeId& prop_decl_id,
                               scada::Variant value);

// TODO: Rename to `SetProperty` for consistency with `SetReference`.
void SetOrAddOrDeleteProperty(NodeProperties& properties,
                              const NodeId& prop_decl_id,
                              scada::Variant value);

void SetProperties(NodeProperties& properties,
                   std::span<const NodeProperty> updated_properties);

[[nodiscard]] scada::ReferenceDescription* FindReference(
    scada::ReferenceDescriptions& references,
    const scada::NodeId& reference_type_id,
    bool forward);

void SetReference(scada::ReferenceDescriptions& references,
                  const scada::ReferenceDescription& updated_reference);

void SetReferences(scada::ReferenceDescriptions& references,
                   std::span<const ReferenceDescription> updated_references);

std::ostream& operator<<(std::ostream& stream, const NodeState& node_state);

}  // namespace scada
