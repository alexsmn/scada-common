#include "node_service/static/static_node_service.h"

#include "base/map_util.h"
#include "model/nested_node_ids.h"
#include "node_service/static/static_node_model.h"

namespace {

void RelocateReferences(scada::NodeState& node_state) {
  if (!node_state.type_definition_id.is_null()) {
    node_state.references.emplace_back(
        /*reference_type_id=*/scada::id::HasTypeDefinition,
        /*forward=*/true,
        /*node_id=*/std::move(node_state.type_definition_id));

    node_state.type_definition_id = {};
  }

  if (!node_state.parent_id.is_null()) {
    assert(!node_state.reference_type_id.is_null());

    node_state.references.emplace_back(
        /*reference_type_id=*/std::move(node_state.reference_type_id),
        /*forward=*/false,
        /*node_id=*/std::move(node_state.parent_id));

    node_state.parent_id = {};
    node_state.reference_type_id = {};
  }
}

}  // namespace

void StaticNodeService::Add(scada::NodeState node_state) {
  assert(!node_state.node_id.is_null());
  assert(scada::IsInstance(node_state.node_class) ^
         node_state.type_definition_id.is_null());

  RelocateReferences(node_state);

  for (const auto& desc : node_state.references) {
    AddInverseReference(node_state.node_id, desc);
  }

  if (scada::IsInstance(node_state.node_class)) {
    for (const auto& [prop_decl_id, prop_value] : node_state.properties) {
      Add(MakePropNodeState(node_state.node_id, prop_decl_id, prop_value));
    }
  }

  node_state.properties.clear();

  auto node_id = node_state.node_id;

  nodes_.try_emplace(std::move(node_id), std::make_shared<StaticNodeModel>(
                                             *this, std::move(node_state)));
}

void StaticNodeService::AddAll(std::span<const scada::NodeState> node_states) {
  for (const auto& node_state : node_states) {
    Add(node_state);
  }
}

NodeRef StaticNodeService::GetNode(const scada::NodeId& node_id) {
  auto i = nodes_.find(node_id);
  return i != nodes_.end() ? NodeRef{i->second} : nullptr;
}

scada::NodeId StaticNodeService::MakeAggregateId(
    const scada::NodeId& node_id,
    const scada::NodeId& prop_decl_id) const {
  return MakeNestedNodeId(node_id, prop_decl_id.ToString());
}

// static
scada::NodeState StaticNodeService::MakePropNodeState(
    const scada::NodeId& node_id,
    const scada::NodeId& prop_decl_id,
    const scada::Variant& prop_value) {
  return {.node_id = MakeAggregateId(node_id, prop_decl_id),
          .node_class = scada::NodeClass::Variable,
          .type_definition_id = scada::id::PropertyType,
          .parent_id = node_id,
          .reference_type_id = scada::id::HasProperty,
          .attributes = {.value = prop_value}};
}

NodeRef::Reference StaticNodeService::GetReference(
    const scada::ReferenceDescription& desc) {
  return {.reference_type = GetNode(desc.reference_type_id),
          .target = GetNode(desc.node_id),
          .forward = desc.forward};
}

const std::set<scada::ReferenceDescription>&
StaticNodeService::inverse_references(const scada::NodeId& node_id) const {
  static const std::set<scada::ReferenceDescription> empty_set;
  auto i = inverse_references_.find(node_id);
  return i != inverse_references_.end() ? i->second : empty_set;
}

void StaticNodeService::AddInverseReference(
    const scada::NodeId& node_id,
    const scada::ReferenceDescription& desc) {
  assert(!node_id.is_null());
  assert(!desc.reference_type_id.is_null());
  assert(!desc.node_id.is_null());

  inverse_references_[desc.node_id].emplace(
      /*reference_type_id=*/desc.reference_type_id,
      /*forward=*/!desc.forward,
      /*node_id=*/node_id);
}