#pragma once

#include "common/node_state.h"
#include "common/node_state_util.h"
#include "node_service/node_model.h"

class TestNodeModel final : public NodeModel {
 public:
  TestNodeModel() {}
  TestNodeModel(const scada::NodeState& node_state) : node_state_{node_state} {}

  virtual scada::Status GetStatus() const override {
    return scada::StatusCode::Good;
  }

  virtual NodeFetchStatus GetFetchStatus() const override {
    return NodeFetchStatus::Max();
  }

  virtual void Fetch(const NodeFetchStatus& requested_status,
                     const FetchCallback& callback) const override {
    assert(false);
  }

  virtual scada::Variant GetAttribute(
      scada::AttributeId attribute_id) const override {
    return scada::Read(node_state_, attribute_id);
  }

  virtual NodeRef GetDataType() const override {
    assert(false);
    return nullptr;
  }

  virtual NodeRef::Reference GetReference(
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& node_id) const override {
    assert(false);
    return {};
  }

  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override {
    assert(false);
    return {};
  }

  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id,
                            bool forward) const override {
    assert(false);
    return nullptr;
  }

  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& reference_type_id,
      bool forward) const override {
    assert(false);
    return {};
  }

  virtual NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const override {
    assert(false);
    return nullptr;
  }

  virtual NodeRef GetChild(
      const scada::QualifiedName& child_name) const override {
    assert(false);
    return nullptr;
  }

  virtual void Subscribe(NodeRefObserver& observer) const override {
    assert(false);
  }

  virtual void Unsubscribe(NodeRefObserver& observer) const override {
    assert(false);
  }

  virtual std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      scada::AttributeId attribute_id,
      const scada::MonitoringParameters& params) const override {
    assert(false);
    return nullptr;
  }

  virtual void Read(scada::AttributeId attribute_id,
                    const NodeRef::ReadCallback& callback) const override {
    assert(false);
  }

  virtual void Write(scada::AttributeId attribute_id,
                     const scada::Variant& value,
                     const scada::WriteFlags& flags,
                     const scada::NodeId& user_id,
                     const scada::StatusCallback& callback) const override {
    assert(false);
  }

  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) const override {
    assert(false);
  }

 private:
  const scada::NodeState node_state_;
};
