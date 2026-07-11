#include "node_service/proxy/proxy_node_service.h"

NodeRef ProxyNodeService::GetNode(const scada::NodeId& node_id) {
  return NodeRef{node_id, this};
}

scada::Status ProxyNodeService::GetStatus(const scada::NodeId& node_id) {
  return scada::StatusCode::Good;
}

NodeFetchStatus ProxyNodeService::GetFetchStatus(const scada::NodeId& node_id) {
  return {};
}

Awaitable<void> ProxyNodeService::Fetch(
    const scada::NodeId& node_id,
    const NodeFetchStatus& requested_status) {
  co_return;
}

void ProxyNodeService::StartFetch(const scada::NodeId& node_id,
                                  const NodeFetchStatus& requested_status) {}

scada::Variant ProxyNodeService::GetAttribute(const scada::NodeId& node_id,
                                              scada::AttributeId attribute_id) {
  return {};
}

NodeRef ProxyNodeService::GetDataType(const scada::NodeId& node_id) {
  return {};
}

NodeRef::Reference ProxyNodeService::GetReference(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward,
    const scada::NodeId& target_id) {
  return {};
}

std::vector<NodeRef::Reference> ProxyNodeService::GetReferences(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward) {
  return {};
}

NodeRef ProxyNodeService::GetTarget(const scada::NodeId& node_id,
                                    const scada::NodeId& reference_type_id,
                                    bool forward) {
  return {};
}

std::vector<NodeRef> ProxyNodeService::GetTargets(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward) {
  return {};
}

NodeRef ProxyNodeService::GetAggregate(
    const scada::NodeId& node_id,
    const scada::NodeId& aggregate_declaration_id) {
  return {};
}

NodeRef ProxyNodeService::GetChild(const scada::NodeId& node_id,
                                   const scada::QualifiedName& child_name) {
  return {};
}

scada::node ProxyNodeService::GetScadaNode(const scada::NodeId& node_id) {
  return {};
}

boost::signals2::scoped_connection ProxyNodeService::SubscribeModelChanged(
    const scada::NodeId& node_id,
    const ModelChangedCallback& callback) {
  return {};
}

boost::signals2::scoped_connection
ProxyNodeService::SubscribeNodeSemanticChanged(
    const scada::NodeId& node_id,
    const NodeSemanticChangedCallback& callback) {
  return {};
}

boost::signals2::scoped_connection ProxyNodeService::SubscribeNodeFetched(
    const scada::NodeId& node_id,
    const NodeFetchedCallback& callback) {
  return {};
}

boost::signals2::scoped_connection ProxyNodeService::SubscribeNodeStateChanged(
    const scada::NodeId& node_id,
    const NodeStateChangedCallback& callback) {
  return {};
}

boost::signals2::scoped_connection ProxyNodeService::SubscribeModelChanged(
    const ModelChangedCallback& callback) const {
  return {};
}

boost::signals2::scoped_connection
ProxyNodeService::SubscribeNodeSemanticChanged(
    const NodeSemanticChangedCallback& callback) const {
  return {};
}

boost::signals2::scoped_connection ProxyNodeService::SubscribeNodeFetched(
    const NodeFetchedCallback& callback) const {
  return {};
}

boost::signals2::scoped_connection ProxyNodeService::SubscribeNodeStateChanged(
    const NodeStateChangedCallback& callback) const {
  return {};
}

size_t ProxyNodeService::GetPendingTaskCount() const {
  return 0;
}
