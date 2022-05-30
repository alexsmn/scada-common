#pragma once

#include "node_service/node_model.h"

class ProxyNodeService;

class ProxyNodeModel : public NodeModel {
 public:
  ProxyNodeModel(ProxyNodeService& node_service, scada::NodeId node_id);

  // NodeModel
  virtual scada::Status GetStatus() const override;
  virtual NodeFetchStatus GetFetchStatus() const override;
  virtual void Fetch(const NodeFetchStatus& requested_status,
                     const FetchCallback& callback) const override;
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
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;
  virtual std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      scada::AttributeId attribute_id,
      const scada::MonitoringParameters& params) const override;
  virtual void Read(scada::AttributeId attribute_id,
                    const NodeRef::ReadCallback& callback) const override;
  virtual void Write(scada::AttributeId attribute_id,
                     const scada::Variant& value,
                     const scada::WriteFlags& flags,
                     const scada::NodeId& user_id,
                     const scada::StatusCallback& callback) const override;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) const override;

 private:
  ProxyNodeService& node_service_;
  const scada::NodeId node_id_;
};
