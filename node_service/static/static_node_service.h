#pragma once

#include "common/node_state.h"
#include "node_service/node_service.h"
#include "scada/data_services.h"

#include <set>

class StaticNodeModel;

class StaticNodeService final : public NodeService {
 public:
  StaticNodeService();
  explicit StaticNodeService(scada::services services);
  explicit StaticNodeService(DataServices data_services);

  void Add(scada::NodeState node_state);
  void AddAll(std::span<const scada::NodeState> node_states);

  // NodeService
  virtual NodeRef GetNode(const scada::NodeId& node_id) override;

  virtual scada::Status GetStatus(const scada::NodeId& node_id) override;
  virtual NodeFetchStatus GetFetchStatus(const scada::NodeId& node_id) override;
  virtual Awaitable<void> Fetch(
      const scada::NodeId& node_id,
      const NodeFetchStatus& requested_status) override;
  virtual void StartFetch(const scada::NodeId& node_id,
                          const NodeFetchStatus& requested_status) override;
  virtual scada::Variant GetAttribute(const scada::NodeId& node_id,
                                      scada::AttributeId attribute_id) override;
  virtual NodeRef GetDataType(const scada::NodeId& node_id) override;
  virtual NodeRef::Reference GetReference(
      const scada::NodeId& node_id,
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& target_id) override;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& node_id,
      const scada::NodeId& reference_type_id,
      bool forward) override;
  virtual NodeRef GetTarget(const scada::NodeId& node_id,
                            const scada::NodeId& reference_type_id,
                            bool forward) override;
  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& node_id,
      const scada::NodeId& reference_type_id,
      bool forward) override;
  virtual NodeRef GetAggregate(
      const scada::NodeId& node_id,
      const scada::NodeId& aggregate_declaration_id) override;
  virtual NodeRef GetChild(const scada::NodeId& node_id,
                           const scada::QualifiedName& child_name) override;
  virtual scada::node GetScadaNode(const scada::NodeId& node_id) override;

  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeModelChanged(
      const scada::NodeId& node_id,
      const ModelChangedCallback& callback) override {
    return {};
  }
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const scada::NodeId& node_id,
      const NodeSemanticChangedCallback& callback) override {
    return {};
  }
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const scada::NodeId& node_id,
      const NodeFetchedCallback& callback) override {
    return {};
  }
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(
      const scada::NodeId& node_id,
      const NodeStateChangedCallback& callback) override {
    return {};
  }

  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeModelChanged(const ModelChangedCallback& callback) const override {
    return {};
  }
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const NodeSemanticChangedCallback& callback) const override {
    return {};
  }
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const NodeFetchedCallback& callback) const override {
    return {};
  }
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(
      const NodeStateChangedCallback& callback) const override {
    return {};
  }
  virtual size_t GetPendingTaskCount() const override { return 0; }

 private:
  std::shared_ptr<const StaticNodeModel> FindNode(
      const scada::NodeId& node_id) const;

  void AddInverseReference(const scada::NodeId& node_id,
                           const scada::ReferenceDescription& desc);

  scada::NodeId MakeAggregateId(const scada::NodeId& node_id,
                                const scada::NodeId& prop_decl_id) const;

  scada::NodeState MakePropNodeState(const scada::NodeId& node_id,
                                     const scada::NodeId& prop_decl_id,
                                     const scada::Variant& prop_value);

  NodeRef::Reference GetReference(const scada::ReferenceDescription& desc);

  const std::set<scada::ReferenceDescription>& inverse_references(
      const scada::NodeId& node_id) const;

  DataServices data_services_;

  std::unordered_map<scada::NodeId, std::shared_ptr<const StaticNodeModel>>
      nodes_;

  std::unordered_map<scada::NodeId, std::set<scada::ReferenceDescription>>
      inverse_references_;

  friend class StaticNodeModel;
};
