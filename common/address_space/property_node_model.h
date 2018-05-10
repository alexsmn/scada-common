#pragma once

#include "common/address_space/address_space_node_model.h"
#include "common/node_id_util.h"

class AddressSpacePropertyModel final : public NodeModel {
 public:
  AddressSpacePropertyModel(
      std::shared_ptr<const AddressSpaceNodeModel> parent_model,
      std::shared_ptr<const NodeModel> declaration_model,
      AddressSpaceNodeModelDelegate& delegate);

  // NodeModel
  virtual scada::Status GetStatus() const override;
  virtual NodeFetchStatus GetFetchStatus() const override;
  virtual void Fetch(const NodeFetchStatus& params,
                     const FetchCallback& callback) const override;
  virtual scada::Variant GetAttribute(
      scada::AttributeId attribute_id) const override;
  virtual NodeRef GetDataType() const override;
  virtual NodeRef::Reference GetReference(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
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
  virtual NodeRef GetAggregate(
      const scada::QualifiedName& aggregate_name) const override;
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;
  virtual std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(
      scada::AttributeId attribute_id) const override;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::StatusCallback& callback) const override;

 private:
  scada::NodeId GetNodeId() const;

  const std::shared_ptr<const AddressSpaceNodeModel> parent_model_;
  const std::shared_ptr<const NodeModel> declaration_model_;
  AddressSpaceNodeModelDelegate& delegate_;
};

inline AddressSpacePropertyModel::AddressSpacePropertyModel(
    std::shared_ptr<const AddressSpaceNodeModel> parent_model,
    std::shared_ptr<const NodeModel> declaration_model,
    AddressSpaceNodeModelDelegate& delegate)
    : parent_model_{std::move(parent_model)},
      declaration_model_{std::move(declaration_model)},
      delegate_{delegate} {
  assert(parent_model_);
  assert(declaration_model_);
}

inline scada::Status AddressSpacePropertyModel::GetStatus() const {
  return parent_model_->GetStatus();
}

inline NodeFetchStatus AddressSpacePropertyModel::GetFetchStatus() const {
  return parent_model_->GetFetchStatus();
}

inline NodeRef AddressSpacePropertyModel::GetDataType() const {
  return declaration_model_->GetDataType();
}

inline NodeRef::Reference AddressSpacePropertyModel::GetReference(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  return {};
}

inline std::vector<NodeRef::Reference> AddressSpacePropertyModel::GetReferences(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  return {};
}

inline NodeRef AddressSpacePropertyModel::GetTarget(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  return nullptr;
}

inline std::vector<NodeRef> AddressSpacePropertyModel::GetTargets(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  return {};
}

inline NodeRef AddressSpacePropertyModel::GetAggregate(
    const scada::NodeId& aggregate_declaration_id) const {
  return nullptr;
}

inline NodeRef AddressSpacePropertyModel::GetAggregate(
    const scada::QualifiedName& aggregate_name) const {
  return nullptr;
}

inline void AddressSpacePropertyModel::Fetch(
    const NodeFetchStatus& params,
    const FetchCallback& callback) const {
  parent_model_->Fetch(params, callback);
}

inline scada::Variant AddressSpacePropertyModel::GetAttribute(
    scada::AttributeId attribute_id) const {
  return parent_model_->GetPropertyAttribute(*declaration_model_, attribute_id);
}

inline void AddressSpacePropertyModel::Subscribe(
    NodeRefObserver& observer) const {
  parent_model_->Subscribe(observer);
}

inline void AddressSpacePropertyModel::Unsubscribe(
    NodeRefObserver& observer) const {
  parent_model_->Unsubscribe(observer);
}

std::unique_ptr<scada::MonitoredItem> inline AddressSpacePropertyModel::
    CreateMonitoredItem(scada::AttributeId attribute_id) const {
  return delegate_.OnNodeModelCreateMonitoredItem({GetNodeId(), attribute_id});
}

inline void AddressSpacePropertyModel::Call(
    const scada::NodeId& method_id,
    const std::vector<scada::Variant>& arguments,
    const scada::StatusCallback& callback) const {
  delegate_.OnNodeModelCall(GetNodeId(), method_id, arguments, {}, callback);
}

inline scada::NodeId AddressSpacePropertyModel::GetNodeId() const {
  return parent_model_
      ->GetPropertyAttribute(*declaration_model_, scada::AttributeId::NodeId)
      .as_node_id();
}
