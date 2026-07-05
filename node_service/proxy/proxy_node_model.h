#pragma once

#include "node_service/node_model.h"

class ProxyNodeService;

class ProxyNodeModel : public NodeModel {
 public:
  ProxyNodeModel(ProxyNodeService& node_service, scada::NodeId node_id);

  // NodeModel
  virtual scada::Status GetStatus() const override;
  virtual NodeFetchStatus GetFetchStatus() const override;
  virtual Awaitable<void> Fetch(
      const NodeFetchStatus& requested_status) const override;
  virtual void StartFetch(
      const NodeFetchStatus& requested_status) const override;
  virtual scada::Variant GetAttribute(
      scada::AttributeId attribute_id) const override;
  virtual NodeRef GetDataType() const override;
  virtual NodeRef::Reference GetReference(
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& node_id) const override;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id,
                            bool forward) const override;
  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  virtual NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const override;
  virtual NodeRef GetChild(
      const scada::QualifiedName& child_name) const override;
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

 private:
  ProxyNodeService& node_service_;
  const scada::NodeId node_id_;
};
