#include "address_space/generic_node_factory.h"

#include "address_space/address_space_impl.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/logger.h"
#include "common/node_id_util.h"
#include "common/node_state.h"
#include "common/node_util.h"
#include "core/standard_node_ids.h"

std::pair<scada::Status, scada::Node*> GenericNodeFactory::CreateNode(
    const scada::NodeState& node_state) {
  logger_->WriteF(
      LogSeverity::Normal,
      "CreateNode [node_id=%s, node_class=%s, type_definition_id=%s, "
      "parent_id=%s, reference_type_id=%s",
      NodeIdToScadaString(node_state.node_id).c_str(),
      ToString(node_state.node_class).c_str(),
      NodeIdToScadaString(node_state.type_definition_id).c_str(),
      NodeIdToScadaString(node_state.parent_id).c_str(),
      NodeIdToScadaString(node_state.reference_type_id).c_str());

  if (node_state.node_id != scada::id::RootFolder) {
    assert(!node_state.parent_id.is_null());
    assert(!node_state.reference_type_id.is_null());
  }

  if (address_space_.GetNode(node_state.node_id))
    throw scada::Status(scada::StatusCode::Bad_DuplicateNodeId);

  auto* type_definition =
      AsTypeDefinition(address_space_.GetNode(node_state.type_definition_id));
  if (!type_definition)
    throw scada::Status(scada::StatusCode::Bad_WrongTypeId);

  std::unique_ptr<scada::Node> node;
  if (node_state.node_class == scada::NodeClass::Object) {
    if (!type_definition ||
        type_definition->GetNodeClass() != scada::NodeClass::ObjectType)
      return {scada::StatusCode::Bad_WrongTypeId, nullptr};
    node = std::make_unique<scada::GenericObject>();

  } else if (node_state.node_class == scada::NodeClass::Variable) {
    auto* variable_type = scada::AsVariableType(type_definition);
    if (!variable_type)
      return {scada::StatusCode::Bad_WrongTypeId, nullptr};
    auto* data_type = scada::AsDataType(
        address_space_.GetNode(node_state.attributes.data_type));
    if (!data_type)
      return {scada::StatusCode::Bad_WrongTypeId, nullptr};
    node = std::make_unique<scada::GenericVariable>(*data_type);

  } else
    return {scada::StatusCode::Bad_WrongNodeClass, nullptr};

  node->set_id(std::move(node_state.node_id));

  scada::AddReference(address_space_, scada::id::HasTypeDefinition, *node,
                      *type_definition);

  if (!node_state.attributes.browse_name.empty())
    node->SetBrowseName(node_state.attributes.browse_name);
  if (!node_state.attributes.display_name.empty())
    node->SetDisplayName(node_state.attributes.display_name);

  for (auto& prop : node_state.properties) {
    /*logger_->WriteF(LogSeverity::Normal, "Property %s[%s] = %s",
                  NodeIdToScadaString(node_state.id).c_str(),
                  NodeIdToScadaString(prop.first).c_str(),
                  prop.second.get_or(std::string{"(unknown)"}).c_str());*/
    auto status =
        scada::SetPropertyValue(*node, prop.first, std::move(prop.second));
    if (!status)
      return {status, nullptr};
  }

  auto* node_ptr = node.get();

  address_space_.AddStaticNode(std::move(node));

  if (!node_state.parent_id.is_null()) {
    auto* parent = address_space_.GetNode(node_state.parent_id);
    if (!parent)
      throw scada::Status(scada::StatusCode::Bad_WrongParentId);

    auto* reference_type =
        AsReferenceType(address_space_.GetNode(node_state.reference_type_id));
    if (!reference_type)
      throw scada::Status(scada::StatusCode::Bad_WrongReferenceId);

    scada::AddReference(*reference_type, *parent, *node_ptr);
  }

  return {scada::StatusCode::Good, node_ptr};
}
