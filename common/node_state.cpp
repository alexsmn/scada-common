#include "common/node_state.h"

#include "base/debug_util.h"

#include <ranges>

#include "base/debug_util-inl.h"

namespace scada {

std::optional<scada::Variant> NodeState::GetAttribute(
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

[[nodiscard]] const ReferenceDescription* FindReference(
    const std::vector<ReferenceDescription>& references,
    const NodeId& reference_type_id,
    bool forward) {
  auto i = std::ranges::find_if(references, [&](auto& p) {
    return p.forward == forward && p.reference_type_id == reference_type_id;
  });
  return i != references.end() ? std::to_address(i) : nullptr;
}

[[nodiscard]] Variant* FindProperty(NodeProperties& properties,
                                    const NodeId& prop_decl_id) {
  auto i = std::ranges::find(properties, prop_decl_id,
                             [&](auto& p) { return p.first; });
  return i != properties.end() ? &i->second : nullptr;
}

// TODO: Rename to `UpdateProperty` or `ReplaceProperty`.
[[nodiscard]] bool SetProperty(NodeProperties& properties,
                               const NodeId& prop_decl_id,
                               scada::Variant value) {
  auto* prop = FindProperty(properties, prop_decl_id);
  if (!prop)
    return false;
  *prop = std::move(value);
  return true;
}

// TODO: Rename to `SetProperty` for consistency with `SetReference`.
void SetOrAddOrDeleteProperty(NodeProperties& properties,
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

void SetProperties(NodeProperties& properties,
                   std::span<const NodeProperty> updated_properties) {
  for (auto& [prop_decl_id, prop_value] : updated_properties) {
    SetOrAddOrDeleteProperty(properties, prop_decl_id, std::move(prop_value));
  }
}

[[nodiscard]] scada::ReferenceDescription* FindReference(
    scada::ReferenceDescriptions& references,
    const scada::NodeId& reference_type_id,
    bool forward) {
  auto i = std::ranges::find_if(references, [&](auto&& ref) {
    return ref.reference_type_id == reference_type_id && ref.forward == forward;
  });
  return i != references.end() ? std::to_address(i) : nullptr;
}

void SetReference(scada::ReferenceDescriptions& references,
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

void SetReferences(scada::ReferenceDescriptions& references,
                   std::span<const ReferenceDescription> updated_references) {
  for (auto& ref : updated_references) {
    SetReference(references, ref);
  }
}

NodeState& NodeState::set_property(const scada::NodeId& prop_decl_id,
                                   scada::Variant value) {
  SetOrAddOrDeleteProperty(properties, prop_decl_id, std::move(value));
  return *this;
}

std::ostream& operator<<(std::ostream& stream, const NodeState& node_state) {
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