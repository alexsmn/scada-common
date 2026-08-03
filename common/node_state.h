#pragma once

#include "base/ostream_formatter.h"
#include "scada/node_attributes.h"
#include "scada/node_class.h"
#include "scada/node_id.h"
#include "scada/variant.h"
#include "scada/view_service.h"

#include <algorithm>
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

  // Construct with designated initializers; there are deliberately no plain
  // field setters. The two helpers below stay because they are not field
  // assignments: `set_property` is find-or-add-or-delete, `add_reference`
  // appends.
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

[[nodiscard]] inline const NodeId* FindReferenceTarget(
    const std::vector<ReferenceDescription>& references,
    const NodeId& reference_type_id,
    bool forward) {
  auto* ref = FindReference(references, reference_type_id, forward);
  return ref ? &ref->node_id : nullptr;
}

[[nodiscard]] Variant* FindProperty(NodeProperties& properties,
                                    const NodeId& prop_decl_id);

[[nodiscard]] inline const Variant* FindProperty(
    const NodeProperties& properties,
    const NodeId& prop_decl_id) {
  return FindProperty(const_cast<NodeProperties&>(properties), prop_decl_id);
}

[[nodiscard]] inline Variant GetProperty(const NodeProperties& properties,
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

// std::format support (used by base::AsList element rendering, e.g. for
// NodeState::children), delegating to the operator<< overload above.
template <>
struct std::formatter<scada::NodeState> : OStreamFormatter {};
