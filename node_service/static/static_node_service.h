#pragma once

#include "common/node_state.h"
#include "node_service/node_service.h"
#include "node_service/static/static_node_model.h"

class StaticNodeModel;

class StaticNodeService : public NodeService {
 public:
  explicit StaticNodeService(scada::services services = {})
      : services_{std::move(services)} {}

  void Add(scada::NodeState node_state) {
    nodes_.try_emplace(node_state.node_id, std::make_shared<StaticNodeModel>(
                                               *this, std::move(node_state)));
  }

  void AddAll(std::span<const scada::NodeState> node_states) {
    std::ranges::for_each(node_states,
                          [this](const auto& node_state) { Add(node_state); });
  }

  // NodeService

  virtual NodeRef GetNode(const scada::NodeId& node_id) override {
    auto i = nodes_.find(node_id);
    return i != nodes_.end() ? NodeRef{i->second} : nullptr;
  }

  virtual void Subscribe(NodeRefObserver& observer) const override {}
  virtual void Unsubscribe(NodeRefObserver& observer) const override {}
  virtual size_t GetPendingTaskCount() const override { return 0; }

 private:
  scada::services services_;

  std::unordered_map<scada::NodeId, std::shared_ptr<const StaticNodeModel>>
      nodes_;

  friend class StaticNodeModel;
};
