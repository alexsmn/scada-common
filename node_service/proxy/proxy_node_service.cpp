#include "node_service/proxy/proxy_node_service.h"

#include "node_service/proxy/proxy_node_model.h"

NodeRef ProxyNodeService::GetNode(const scada::NodeId& node_id) {
  auto i = proxy_node_models_.find(node_id);
  if (i != proxy_node_models_.end())
    return NodeRef{i->second};

  return NodeRef{};
}

void ProxyNodeService::Subscribe(NodeRefObserver& observer) const {}

void ProxyNodeService::Unsubscribe(NodeRefObserver& observer) const {}

size_t ProxyNodeService::GetPendingTaskCount() const {
  return 0;
}
