#pragma once

#include "address_space/address_space_impl3.h"
#include "address_space/node.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"

inline scada::NodeState GetNodeState(const scada::Node& node) {
  scada::NodeState node_state;

  node_state.node_id = node.id();
  node_state.node_class = node.GetNodeClass();
  node_state.type_definition_id = scada::GetNodeId(node.type_definition());
  node_state.attributes.browse_name = node.GetBrowseName();
  node_state.attributes.display_name = node.GetDisplayName();

  auto [reference_type, parent] = scada::GetParentReference(node);
  node_state.parent_id = scada::GetNodeId(parent);
  node_state.reference_type_id = scada::GetNodeId(reference_type);

  if (scada::IsTypeDefinition(node.GetNodeClass())) {
    const auto& type_definition =
        static_cast<const scada::TypeDefinition&>(node);
    node_state.supertype_id = scada::GetNodeId(type_definition.supertype());
  }

  if (node.GetNodeClass() == scada::NodeClass::Variable) {
    const auto& variable = static_cast<const scada::Variable&>(node);
    node_state.attributes.data_type = variable.GetDataType().id();
    node_state.attributes.value = variable.GetValue().value;
  }

  /*for (const auto& prop : scada::GetProperties(node)) {
    auto prop_decl_id = scada::GetDeclarationId(prop);
    assert(!prop_decl_id.is_null());
    node_state.properties.emplace_back(std::move(prop_decl_id),
                                       prop.GetValue().value);
  }*/

  return node_state;
}

inline std::vector<scada::NodeState> GetNodeStates(
    const AddressSpaceImpl& address_space) {
  std::vector<scada::NodeState> node_states;
  node_states.reserve(address_space.node_map().size());

  for (const auto& [_, node] : address_space.node_map()) {
    node_states.emplace_back(GetNodeState(*node));
  }

  return node_states;
}

inline std::vector<scada::NodeState> GetScadaNodeStates() {
  return GetNodeStates(AddressSpaceImpl3{});
}
