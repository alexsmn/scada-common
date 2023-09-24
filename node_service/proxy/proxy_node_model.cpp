#include "node_service/proxy/proxy_node_model.h"

ProxyNodeModel::ProxyNodeModel(ProxyNodeService& node_service,
                               scada::NodeId node_id)
    : node_service_{node_service}, node_id_{std::move(node_id)} {}

scada::Status ProxyNodeModel::GetStatus() const {
  return scada::StatusCode::Good;
}

NodeFetchStatus ProxyNodeModel::GetFetchStatus() const {
  return {};
}

void ProxyNodeModel::Fetch(const NodeFetchStatus& requested_status,
                           const FetchCallback& callback) const {}

scada::Variant ProxyNodeModel::GetAttribute(
    scada::AttributeId attribute_id) const {
  return {};
}

NodeRef ProxyNodeModel::GetDataType() const {
  return {};
}

NodeRef::Reference ProxyNodeModel::GetReference(
    const scada::NodeId& reference_type_id,
    bool forward,
    const scada::NodeId& node_id) const {
  return {};
}

std::vector<NodeRef::Reference> ProxyNodeModel::GetReferences(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  return {};
}

NodeRef ProxyNodeModel::GetTarget(const scada::NodeId& reference_type_id,
                                  bool forward) const {
  return {};
}

std::vector<NodeRef> ProxyNodeModel::GetTargets(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  return {};
}

NodeRef ProxyNodeModel::GetAggregate(
    const scada::NodeId& aggregate_declaration_id) const {
  return {};
}

NodeRef ProxyNodeModel::GetChild(const scada::QualifiedName& child_name) const {
  return {};
}

void ProxyNodeModel::Subscribe(NodeRefObserver& observer) const {}

void ProxyNodeModel::Unsubscribe(NodeRefObserver& observer) const {}
