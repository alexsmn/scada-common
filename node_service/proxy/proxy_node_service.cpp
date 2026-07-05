#include "node_service/proxy/proxy_node_service.h"

#include "node_service/proxy/proxy_node_model.h"

NodeRef ProxyNodeService::GetNode(const scada::NodeId& node_id) {
  auto i = proxy_node_models_.find(node_id);
  if (i != proxy_node_models_.end())
    return NodeRef{i->second};

  return NodeRef{};
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
