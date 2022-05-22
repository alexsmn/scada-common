#pragma once

#include "common/node_state.h"
#include "common/node_state_util.h"
#include "node_service/node_model.h"

class TestNodeModel;

class TestNodeService {
 public:
  std::shared_ptr<TestNodeModel> GetNodeModel(const scada::NodeId& node_id);

  NodeRef GetNode(const scada::NodeId& node_id) {
    return GetNodeModel(node_id);
  }
};

class TestNodeModel final : public NodeModel {
 public:
  TestNodeModel() {}

  explicit TestNodeModel(TestNodeService* node_service)
      : node_service_{node_service} {}

  TestNodeModel(TestNodeService* node_service, scada::NodeState node_state)
      : node_service_{node_service}, node_state{std::move(node_state)} {}

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
    return scada::ReadAttribute(node_state, attribute_id)
        .value_or(scada::Variant{});
  }

  virtual NodeRef GetDataType() const override {
    assert(false);
    return nullptr;
  }

  virtual NodeRef::Reference GetReference(
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& node_id) const override {
    return {};
  }

  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override {
    return {};
  }

  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id,
                            bool forward) const override {
    if (!node_service_)
      return nullptr;

    if (forward && reference_type_id == scada::id::HasTypeDefinition)
      return node_service_->GetNode(node_state.type_definition_id);

    return nullptr;
  }

  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& reference_type_id,
      bool forward) const override {
    return {};
  }

  virtual NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const override {
    return nullptr;
  }

  virtual NodeRef GetChild(
      const scada::QualifiedName& child_name) const override {
    assert(false);
    return nullptr;
  }

  virtual void Subscribe(NodeRefObserver& observer) const override {}

  virtual void Unsubscribe(NodeRefObserver& observer) const override {}

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

  scada::NodeState node_state;

 private:
  TestNodeService* node_service_ = nullptr;
};

inline std::shared_ptr<TestNodeModel> TestNodeService::GetNodeModel(
    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  return std::make_shared<TestNodeModel>(this);
}
