#include "node_service/static/static_node_model.h"

#include "common/variable_handle.h"
#include "core/monitored_item_service.h"
#include "model/node_id_util.h"
#include "node_service/static/static_node_service.h"

StaticNodeModel::StaticNodeModel(StaticNodeService& service,
                                 scada::NodeState node_state)
    : service_{service}, node_state_{std::move(node_state)} {}

scada::Variant StaticNodeModel::GetAttribute(
    scada::AttributeId attribute_id) const {
  return node_state_.GetAttribute(attribute_id).value_or(scada::Variant{});
}

NodeRef StaticNodeModel::GetDataType() const {
  return service_.GetNode(node_state_.attributes.data_type);
}

NodeRef::Reference StaticNodeModel::GetReference(
    const scada::NodeId& reference_type_id,
    bool forward,
    const scada::NodeId& node_id) const {
  return {};
}

std::vector<NodeRef::Reference> StaticNodeModel::GetReferences(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  return {};
}

NodeRef StaticNodeModel::GetTarget(const scada::NodeId& reference_type_id,
                                   bool forward) const {
  if (forward && reference_type_id == scada::id::HasTypeDefinition) {
    return std::make_shared<StaticNodeModel>(
        service_, scada::NodeState{.node_id = node_state_.type_definition_id});
  }

  if (!forward && reference_type_id == scada::id::HierarchicalReferences) {
    return service_.GetNode(node_state_.parent_id);
  }

  auto* target = scada::FindReferenceTarget(node_state_.references,
                                            reference_type_id, forward);
  return target ? service_.GetNode(*target) : NodeRef{};
}

std::vector<NodeRef> StaticNodeModel::GetTargets(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef> targets;

  if (forward && reference_type_id == scada::id::HasProperty) {
    for (auto& [prop_decl_id, value] : node_state_.properties) {
      targets.emplace_back(service_.GetNode(prop_decl_id));
    }
  }

  return targets;
}

NodeRef StaticNodeModel::GetAggregate(
    const scada::NodeId& aggregate_declaration_id) const {
  auto* prop =
      scada::FindProperty(node_state_.properties, aggregate_declaration_id);
  return prop ? std::make_shared<StaticNodeModel>(
                    service_,
                    scada::NodeState{
                        .node_id = MakeNestedNodeId(
                            node_state_.node_id,
                            NodeIdToScadaString(aggregate_declaration_id)),
                        .node_class = scada::NodeClass::Variable,
                        .type_definition_id = scada::id::PropertyType,
                        .attributes = {.value = *prop}})
              : NodeRef{};
}

NodeRef StaticNodeModel::GetChild(
    const scada::QualifiedName& child_name) const {
  return {};
}

std::shared_ptr<scada::MonitoredItem> StaticNodeModel::CreateMonitoredItem(
    scada::AttributeId attribute_id,
    const scada::MonitoringParameters& params) const {
  if (service_.services_.monitored_item_service) {
    if (auto monitored_item =
            service_.services_.monitored_item_service->CreateMonitoredItem(
                {node_state_.node_id, attribute_id}, params)) {
      return monitored_item;
    }
  }

  if (attribute_id == scada::AttributeId::Value &&
      node_state_.node_class == scada::NodeClass::Variable) {
    const auto variable = std::make_shared<scada::VariableHandle>();

    if (const auto& optional_value = node_state_.attributes.value;
        optional_value.has_value()) {
      variable->set_last_value(scada::MakeReadResult(optional_value.value()));
    }

    return scada::CreateMonitoredVariable(variable);
  }

  return nullptr;
}

void StaticNodeModel::Read(scada::AttributeId attribute_id,
                           const NodeRef::ReadCallback& callback) const {
  callback(scada::MakeReadResult(
      node_state_.attributes.value.value_or(scada::Variant{})));
}

void StaticNodeModel::Write(scada::AttributeId attribute_id,
                            const scada::Variant& value,
                            const scada::WriteFlags& flags,
                            const scada::NodeId& user_id,
                            const scada::StatusCallback& callback) const {
  callback(scada::StatusCode::Bad);
}

void StaticNodeModel::Call(const scada::NodeId& method_id,
                           const std::vector<scada::Variant>& arguments,
                           const scada::NodeId& user_id,
                           const scada::StatusCallback& callback) const {
  callback(scada::StatusCode::Bad);
}
