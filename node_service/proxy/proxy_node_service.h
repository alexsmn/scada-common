#pragma once

#include "base/any_executor.h"
#include "node_service/node_service.h"

#include <unordered_map>

class ProxyNodeModel;

class ProxyNodeService : public NodeService {
 public:
  virtual NodeRef GetNode(const scada::NodeId& node_id) override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeModelChanged(const ModelChangedCallback& callback) const override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const NodeSemanticChangedCallback& callback) const override;
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const NodeFetchedCallback& callback) const override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(
      const NodeStateChangedCallback& callback) const override;
  virtual size_t GetPendingTaskCount() const override;

 private:
  AnyExecutor executor_;
  const std::shared_ptr<NodeService> source_node_service_;

  std::unordered_map<scada::NodeId, std::shared_ptr<ProxyNodeModel>>
      proxy_node_models_;
};
