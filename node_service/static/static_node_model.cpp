#include "node_service/static/static_node_model.h"

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

scada::node StaticNodeModel::GetScadaNode() const {
  if (const auto& node_id =
          GetAttribute(scada::AttributeId::NodeId).as_node_id();
      !node_id.is_null()) {
    return scada::client{service_.services_}.node(node_id);
  } else {
    return {};
  }
}
