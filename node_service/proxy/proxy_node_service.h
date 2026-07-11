#pragma once

#include "base/any_executor.h"
#include "node_service/node_service.h"

class ProxyNodeService : public NodeService {
 public:
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
      const ModelChangedCallback& callback) override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeSemanticChanged(
      const scada::NodeId& node_id,
      const NodeSemanticChangedCallback& callback) override;
  [[nodiscard]] virtual boost::signals2::scoped_connection SubscribeNodeFetched(
      const scada::NodeId& node_id,
      const NodeFetchedCallback& callback) override;
  [[nodiscard]] virtual boost::signals2::scoped_connection
  SubscribeNodeStateChanged(
      const scada::NodeId& node_id,
      const NodeStateChangedCallback& callback) override;

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
};
