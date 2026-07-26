#include "address_space/node_factory_util.h"
#include "base/check.h"

#include "address_space/node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "common/node_state.h"
#include "model/node_id_util.h"

namespace {

// Placeholder InstanceDeclarations (OptionalPlaceholder/MandatoryPlaceholder)
// describe what instances MAY add; they are not fixed children and must never
// be materialized as a real "<Name>" node when instantiating a type.
bool IsPlaceholderDeclaration(const scada::Node& node) {
  const scada::NodeId rule_id = scada::GetModellingRuleId(node);
  return rule_id ==
             scada::NodeId{scada::id::ModellingRule_OptionalPlaceholder} ||
         rule_id ==
             scada::NodeId{scada::id::ModellingRule_MandatoryPlaceholder};
}

}  // namespace

scada::Status CreateMissingProperties(
    NodeFactory& node_factory,
    const scada::NodeId& node_id,
    const scada::TypeDefinition& type_definition) {
  for (auto* type = &type_definition; type; type = type->supertype()) {
    for (const auto& prop_node : scada::GetProperties(*type)) {
      if (IsPlaceholderDeclaration(prop_node))
        continue;
      auto& prop_decl = scada::AsVariable(prop_node);
      const auto& prop_name = prop_decl.GetBrowseName();
      if (!scada::FindChild(prop_node, prop_name.name())) {
        auto prop_id = MakeNestedNodeId(node_id, prop_name.name());
        auto [status, prop] = node_factory.CreateNode(scada::NodeState{
            std::move(prop_id), scada::NodeClass::Variable,
            scada::id::PropertyType, node_id, scada::id::HasProperty,
            scada::NodeAttributes{.browse_name = prop_decl.GetBrowseName(),
                                  .display_name = prop_decl.GetDisplayName(),
                                  .data_type = prop_decl.GetDataType().id(),
                                  .value = prop_decl.GetValue().value}});
        if (!status)
          return status;
      }
    }
  }
  return scada::StatusCode::Good;
}

scada::Status CreateMissingChildren(
    NodeFactory& node_factory,
    const scada::NodeId& node_id,
    const scada::TypeDefinition& type_definition) {
  for (auto* type = &type_definition; type; type = type->supertype()) {
    for (const auto* component_node : scada::GetComponents(*type)) {
      if (IsPlaceholderDeclaration(*component_node))
        continue;

      const scada::NodeClass node_class = component_node->GetNodeClass();
      if (node_class != scada::NodeClass::Variable &&
          node_class != scada::NodeClass::Object &&
          node_class != scada::NodeClass::Method) {
        continue;
      }

      const scada::QualifiedName& browse_name =
          component_node->GetBrowseName();
      if (scada::FindChild(*component_node, browse_name.name()))
        continue;

      // A Method node has no type definition, and OPC UA does not give it one.
      // Only Variable and Object declarations carry one, and a declaration
      // missing it is a defect in the nodeset, not a reason to abort the
      // process — skip it rather than Check-failing as this code used to.
      scada::NodeId type_definition_id;
      if (node_class != scada::NodeClass::Method) {
        const auto* component_type = component_node->type_definition();
        if (!component_type)
          continue;
        type_definition_id = component_type->id();
      }

      scada::NodeAttributes attributes{
          .browse_name = browse_name,
          .display_name = component_node->GetDisplayName()};
      // Only a Variable carries a data type and a value.
      if (const auto* variable_decl = scada::AsVariable(component_node)) {
        attributes.data_type = variable_decl->GetDataType().id();
        attributes.value = variable_decl->GetValue().value;
      }

      auto [status, component] = node_factory.CreateNode(scada::NodeState{
          MakeNestedNodeId(node_id, browse_name.name()), node_class,
          std::move(type_definition_id), node_id, scada::id::HasComponent,
          std::move(attributes)});
      if (!status)
        return status;
    }
  }
  return scada::StatusCode::Good;
}
