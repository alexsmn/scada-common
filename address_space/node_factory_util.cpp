#include "address_space/node_factory_util.h"

#include "address_space/node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "common/node_state.h"
#include "model/node_id_util.h"

void CreateMissingProperties(NodeFactory& node_factory,
                             const scada::NodeId& node_id,
                             const scada::TypeDefinition& type_definition) {
  for (auto* type = &type_definition; type; type = type->supertype()) {
    for (const auto& prop_node : scada::GetProperties(*type)) {
      auto& prop_decl = scada::AsVariable(prop_node);
      const auto& prop_name = prop_decl.GetBrowseName();
      if (!scada::FindChild(prop_node, prop_name.name())) {
        auto prop_id = MakeNestedNodeId(node_id, prop_name.name());
        auto [status, prop] = node_factory.CreateNode(scada::NodeState{
            std::move(prop_id), scada::NodeClass::Variable,
            scada::id::PropertyType, node_id, scada::id::HasProperty,
            scada::NodeAttributes{}
                .set_browse_name(prop_decl.GetBrowseName())
                .set_display_name(prop_decl.GetDisplayName())
                .set_data_type(prop_decl.GetDataType().id())
                .set_value(prop_decl.GetValue().value)});
        if (!status)
          throw status;
      }
    }
  }
}

void CreateDataVariables(NodeFactory& node_factory,
                         const scada::NodeId& node_id,
                         const scada::TypeDefinition& type_definition) {
  for (auto* type = &type_definition; type; type = type->supertype()) {
    for (const auto* data_variable_node : scada::GetComponents(*type)) {
      auto& data_variable_decl = scada::AsVariable(*data_variable_node);
      assert(data_variable_decl.type_definition());
      auto data_variable_id =
          MakeNestedNodeId(node_id, data_variable_decl.GetBrowseName().name());
      auto [status, data_variable] = node_factory.CreateNode(scada::NodeState{
          std::move(data_variable_id), scada::NodeClass::Variable,
          data_variable_decl.type_definition()->id(), node_id,
          scada::id::HasComponent,
          scada::NodeAttributes{}
              .set_browse_name(data_variable_decl.GetBrowseName())
              .set_display_name(data_variable_decl.GetDisplayName())
              .set_data_type(data_variable_decl.GetDataType().id())
              .set_value(data_variable_decl.GetValue().value)});
      if (!status)
        throw status;
    }
  }
}
