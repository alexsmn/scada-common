#pragma once

#include "common/node_state.h"
#include "node_service/base_node_model.h"

class StaticNodeService;

class StaticNodeModel : public BaseNodeModel {
 public:
  StaticNodeModel(StaticNodeService& service, scada::NodeState node_state);

  // BaseNodeModel
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
  StaticNodeService& service_;
  const scada::NodeState node_state_;
};
