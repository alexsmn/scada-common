#pragma once

#include "common/node_state.h"
#include "node_service/node_service.h"

#include <set>

class StaticNodeModel;

class StaticNodeService final : public NodeService {
 public:
  explicit StaticNodeService(scada::services services = {})
      : services_{std::move(services)} {}

  void Add(scada::NodeState node_state);
  void AddAll(std::span<const scada::NodeState> node_states);

  // NodeService
  virtual NodeRef GetNode(const scada::NodeId& node_id) override;
  virtual void Subscribe(NodeRefObserver& observer) const override {}
  virtual void Unsubscribe(NodeRefObserver& observer) const override {}
  virtual size_t GetPendingTaskCount() const override { return 0; }

 private:
  void AddInverseReference(const scada::NodeId& node_id,
                           const scada::ReferenceDescription& desc);

  scada::NodeId MakeAggregateId(const scada::NodeId& node_id,
                                const scada::NodeId& prop_decl_id) const;

  scada::NodeState MakePropNodeState(const scada::NodeId& node_id,
                                     const scada::NodeId& prop_decl_id,
                                     const scada::Variant& prop_value);

  NodeRef GetAggregate(const scada::NodeId& node_id,
                       const scada::NodeId& prop_decl_id) {
    return GetNode(MakeAggregateId(node_id, prop_decl_id));
  }

  NodeRef::Reference GetReference(const scada::ReferenceDescription& desc);

  const std::set<scada::ReferenceDescription>& inverse_references(
      const scada::NodeId& node_id) const;

  scada::services services_;

  std::unordered_map<scada::NodeId, std::shared_ptr<const StaticNodeModel>>
      nodes_;

  std::unordered_map<scada::NodeId, std::set<scada::ReferenceDescription>>
      inverse_references_;

  friend class StaticNodeModel;
};
