#include "attribute_service_impl.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/range_util.h"
#include "base/span_util.h"

#include <boost/range/adaptor/transformed.hpp>

SyncAttributeServiceImpl::SyncAttributeServiceImpl(
    AttributeServiceImplContext&& context)
    : AttributeServiceImplContext{std::move(context)} {}

AttributeServiceImpl::AttributeServiceImpl(
    SyncAttributeService& sync_attribute_service)
    : sync_attribute_service_{sync_attribute_service} {}

void AttributeServiceImpl::Read(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  assert(context);
  assert(inputs);

  auto results = sync_attribute_service_.Read(*context, *inputs);
  assert(results.size() == inputs->size());

  callback(scada::StatusCode::Good, std::move(results));
}

void AttributeServiceImpl::Write(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  assert(context);
  assert(inputs);

  auto results = sync_attribute_service_.Write(*context, *inputs);
  callback(scada::StatusCode::Good, std::move(results));
}

std::vector<scada::DataValue> SyncAttributeServiceImpl::Read(
    const scada::ServiceContext& context,
    std::span<const scada::ReadValueId> inputs) {
  return AsBaseSpan(inputs) |
         boost::adaptors::transformed(
             [this](const scada::ReadValueId& input) { return Read(input); }) |
         to_vector;
}

scada::DataValue SyncAttributeServiceImpl::Read(
    const scada::ReadValueId& input) {
  std::string_view nested_name;
  auto* node = scada::GetNestedNode(address_space_, input.node_id, nested_name);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};

  if (nested_name.empty())
    return ReadNode(*node, input.attribute_id);

  return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};
}

std::vector<scada::StatusCode> SyncAttributeServiceImpl::Write(
    const scada::ServiceContext& context,
    std::span<const scada::WriteValue> inputs) {
  return std::vector<scada::StatusCode>(inputs.size(),
                                        scada::StatusCode::Bad_WrongNodeId);
}

scada::DataValue SyncAttributeServiceImpl::ReadNode(
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
