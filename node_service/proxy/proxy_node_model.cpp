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

std::shared_ptr<scada::MonitoredItem> ProxyNodeModel::CreateMonitoredItem(
    scada::AttributeId attribute_id,
    const scada::MonitoringParameters& params) const {
  return {};
}

void ProxyNodeModel::Read(scada::AttributeId attribute_id,
                          const NodeRef::ReadCallback& callback) const {}

void ProxyNodeModel::Write(scada::AttributeId attribute_id,
                           const scada::Variant& value,
                           const scada::WriteFlags& flags,
                           const scada::NodeId& user_id,
                           const scada::StatusCallback& callback) const {}

void ProxyNodeModel::Call(const scada::NodeId& method_id,
                          const std::vector<scada::Variant>& arguments,
                          const scada::NodeId& user_id,
                          const scada::StatusCallback& callback) const {}
