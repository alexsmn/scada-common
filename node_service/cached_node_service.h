#pragma once

#include "node_service/node_service.h"

class cached_node_service {
 public:
  explicit cached_node_service(std::shared_ptr<NodeService> node_service)
      : node_service_{std::move(node_service)} {}

  NodeRef node(const scada::NodeId& node_id) {
    return node_service_->GetNode(node_id);
  }

  void subscribe(NodeRefObserver& observer) const {
    node_service_->Subscribe(observer);
  }

  void unsubscribe(NodeRefObserver& observer) const {
    node_service_->Unsubscribe(observer);
  }

  size_t pending_task_count() const {
    return node_service_->GetPendingTaskCount();
  }

 private:
  std::shared_ptr<NodeService> node_service_;
};