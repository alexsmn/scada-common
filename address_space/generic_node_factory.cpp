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
  return CreateNodeHelper(node_state, node_state.parent_id);
}

std::pair<scada::Status, scada::Node*> GenericNodeFactory::CreateNodeHelper(
    const scada::NodeState& node_state,
    const scada::NodeId& parent_id) {
  assert(!node_state.node_id.is_null());

  if (node_state.node_id != scada::id::RootFolder) {
    assert(!parent_id.is_null());
    assert(!node_state.reference_type_id.is_null());
  }

  if (address_space_.GetNode(node_state.node_id))
    throw scada::Status(scada::StatusCode::Bad_DuplicateNodeId);

  auto* type_definition =
      AsTypeDefinition(address_space_.GetNode(node_state.type_definition_id));

  std::unique_ptr<scada::Node> node;
  if (node_state.node_class == scada::NodeClass::Object) {
    auto* object_type = scada::AsObjectType(type_definition);
    assert(object_type);
    if (!object_type)
      return {scada::StatusCode::Bad_WrongTypeId, nullptr};

    node = std::make_unique<scada::GenericObject>();

  } else if (node_state.node_class == scada::NodeClass::Variable) {
    assert(!node_state.attributes.data_type.is_null());

    auto* variable_type = scada::AsVariableType(type_definition);
    assert(variable_type);
    if (!variable_type)
      return {scada::StatusCode::Bad_WrongTypeId, nullptr};

    auto* data_type = scada::AsDataType(
        address_space_.GetNode(node_state.attributes.data_type));
    assert(data_type);
    if (!data_type)
      return {scada::StatusCode::Bad_WrongTypeId, nullptr};

    node = std::make_unique<scada::GenericVariable>(*data_type);

  } else if (node_state.node_class == scada::NodeClass::ObjectType) {
    assert(!type_definition);

    node = std::make_unique<scada::ObjectType>();

  } else if (node_state.node_class == scada::NodeClass::VariableType) {
    assert(!type_definition);
    assert(!node_state.attributes.data_type.is_null());

    auto* data_type = scada::AsDataType(
        address_space_.GetNode(node_state.attributes.data_type));
    assert(data_type);
    if (!data_type)
      return {scada::StatusCode::Bad_WrongTypeId, nullptr};

    node = std::make_unique<scada::VariableType>(*data_type);

  } else if (node_state.node_class == scada::NodeClass::ReferenceType) {
    assert(!type_definition);

    node = std::make_unique<scada::ReferenceType>();

  } else if (node_state.node_class == scada::NodeClass::DataType) {
    assert(!type_definition);

    node = std::make_unique<scada::DataType>();

  } else {
    assert(false);
    return {scada::StatusCode::Bad_WrongNodeClass, nullptr};
  }

  node->set_id(std::move(node_state.node_id));
  if (!node_state.attributes.browse_name.empty())
    node->SetBrowseName(node_state.attributes.browse_name);
  if (!node_state.attributes.display_name.empty())
    node->SetDisplayName(node_state.attributes.display_name);

  if (type_definition) {
    scada::AddReference(address_space_, scada::id::HasTypeDefinition, *node,
                        *type_definition);
  }

  for (auto& [prop_decl_id, value] : node_state.properties) {
    auto status =
        scada::SetPropertyValue(*node, prop_decl_id, std::move(value));
    if (!status)
      return {status, nullptr};
  }

  auto* node_ptr = node.get();

  address_space_.AddStaticNode(std::move(node));

  if (!parent_id.is_null()) {
    auto* parent = address_space_.GetNode(parent_id);
    if (!parent)
      throw scada::Status(scada::StatusCode::Bad_WrongParentId);

    auto* reference_type =
        AsReferenceType(address_space_.GetNode(node_state.reference_type_id));
    if (!reference_type)
      throw scada::Status(scada::StatusCode::Bad_WrongReferenceId);

    scada::AddReference(*reference_type, *parent, *node_ptr);
  }

  for (auto& child : node_state.children) {
    auto [status, node] = CreateNodeHelper(child, node_state.node_id);
    if (!status)
      throw status;
  }

  return {scada::StatusCode::Good, node_ptr};
}
