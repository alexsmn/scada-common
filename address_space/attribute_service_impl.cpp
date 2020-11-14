#include "attribute_service_impl.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"

#include <atomic>
#include <unordered_map>

// TODO: Remove
#include "model/scada_node_ids.h"

AttributeServiceImpl::AttributeServiceImpl(
    AttributeServiceImplContext&& context)
    : AttributeServiceImplContext{std::move(context)} {}

void AttributeServiceImpl::Read(
    const std::vector<scada::ReadValueId>& descriptions,
    const scada::ReadCallback& callback) {
  std::vector<scada::DataValue> results(descriptions.size());

  for (size_t index = 0; index < descriptions.size(); ++index)
    results[index] = Read(descriptions[index]);

  return callback(scada::StatusCode::Good, std::move(results));
}

void AttributeServiceImpl::Write(
    const std::vector<scada::WriteValueId>& value_ids,
    const scada::NodeId& user_id,
    const scada::WriteCallback& callback) {
  callback(scada::StatusCode::Bad_WrongNodeId, {});
}

scada::DataValue AttributeServiceImpl::Read(const scada::ReadValueId& read_id) {
  std::string_view nested_name;
  auto* node =
      scada::GetNestedNode(address_space_, read_id.node_id, nested_name);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};

  if (nested_name.empty())
    return ReadNode(*node, read_id.attribute_id);

  return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};
}

scada::DataValue AttributeServiceImpl::ReadNode(
    const scada::Node& node,
    scada::AttributeId attribute_id) {
  switch (attribute_id) {
    case scada::AttributeId::NodeClass:
      return scada::MakeReadResult(static_cast<int>(node.GetNodeClass()));

    case scada::AttributeId::BrowseName:
      assert(!node.GetBrowseName().empty());
      return scada::MakeReadResult(node.GetBrowseName());

    case scada::AttributeId::DisplayName:
      return scada::MakeReadResult(node.GetDisplayName());
  }

  if (auto* variable = scada::AsVariable(&node)) {
    switch (attribute_id) {
      case scada::AttributeId::DataType:
        return scada::MakeReadResult(variable->GetDataType().id());
      case scada::AttributeId::Value:
        return variable->GetValue();
    }
  }

  if (auto* variable_type = scada::AsVariableType(&node)) {
    switch (attribute_id) {
      case scada::AttributeId::DataType:
        return scada::MakeReadResult(variable_type->data_type().id());
      case scada::AttributeId::Value:
        return scada::MakeReadResult(variable_type->default_value());
    }
  }

  return {scada::StatusCode::Bad_WrongAttributeId, scada::DateTime::Now()};
}
