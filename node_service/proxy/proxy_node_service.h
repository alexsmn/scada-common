#pragma once

#include "node_service/node_service.h"

#include <unordered_map>

class Executor;
class ProxyNodeModel;

class ProxyNodeService : public NodeService {
 public:
  virtual NodeRef GetNode(const scada::NodeId& node_id) override;
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;
  virtual size_t GetPendingTaskCount() const override;

 private:
  const std::shared_ptr<Executor> executor_;
  const std::shared_ptr<NodeService> source_node_service_;

  std::unordered_map<scada::NodeId, std::shared_ptr<ProxyNodeModel>>
      proxy_node_models_;
};
