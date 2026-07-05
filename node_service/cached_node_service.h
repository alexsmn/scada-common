#pragma once

#include "node_service/node_service.h"

class cached_node_service {
 public:
  explicit cached_node_service(std::shared_ptr<NodeService> node_service)
      : node_service_{std::move(node_service)} {}

  NodeRef node(const scada::NodeId& node_id) {
    return node_service_->GetNode(node_id);
  }

  [[nodiscard]] boost::signals2::scoped_connection subscribe_model_changed(
      const ModelChangedCallback& callback) const {
    return node_service_->SubscribeModelChanged(callback);
  }

  [[nodiscard]] boost::signals2::scoped_connection
  subscribe_node_semantic_changed(
      const NodeSemanticChangedCallback& callback) const {
    return node_service_->SubscribeNodeSemanticChanged(callback);
  }

  [[nodiscard]] boost::signals2::scoped_connection subscribe_node_fetched(
      const NodeFetchedCallback& callback) const {
    return node_service_->SubscribeNodeFetched(callback);
  }

  [[nodiscard]] boost::signals2::scoped_connection subscribe_node_state_changed(
      const NodeStateChangedCallback& callback) const {
    return node_service_->SubscribeNodeStateChanged(callback);
  }

  size_t pending_task_count() const {
    return node_service_->GetPendingTaskCount();
  }

 private:
  std::shared_ptr<NodeService> node_service_;
};