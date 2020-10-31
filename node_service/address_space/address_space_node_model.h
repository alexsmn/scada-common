#pragma once

#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "node_service/base_node_model.h"

namespace scada {
class AttributeService;
class MethodService;
class MonitoredItemService;
struct ModelChangeEvent;
}  // namespace scada

class AddressSpaceNodeModel;

class AddressSpaceNodeModelDelegate {
 public:
  virtual ~AddressSpaceNodeModelDelegate() {}

  // If |node| is nullptr, empty node model is returned.
  virtual NodeRef GetRemoteNode(const scada::Node* node) = 0;

  virtual void OnNodeModelDeleted(const scada::NodeId& node_id) = 0;

  virtual void OnNodeModelFetchRequested(
      const scada::NodeId& node_id,
      const NodeFetchStatus& requested_status) = 0;
};

struct AddressSpaceNodeModelContext {
  AddressSpaceNodeModelDelegate& delegate_;
  const scada::NodeId node_id_;
  scada::AttributeService& attribute_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::MethodService& method_service_;
};

class AddressSpaceNodeModel final
    : private AddressSpaceNodeModelContext,
      public BaseNodeModel,
      public std::enable_shared_from_this<AddressSpaceNodeModel> {
 public:
  explicit AddressSpaceNodeModel(AddressSpaceNodeModelContext&& context);
  ~AddressSpaceNodeModel();

  void OnModelChanged(const scada::ModelChangeEvent& event);
  void OnNodeSemanticChanged();

  void SetFetchStatus(const scada::Node* node,
                      const scada::Status& status,
                      const NodeFetchStatus& fetch_status);

  // NodeModel
  virtual scada::Variant GetAttribute(
      scada::AttributeId attribute_id) const override;
  virtual NodeRef::Reference GetReference(
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& node_id) const override;
  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id,
                            bool forward) const override;
  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  virtual NodeRef GetDataType() const override;
  virtual NodeRef GetAggregate(
      const scada::NodeId& declaration_id) const override;
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
  virtual void OnNodeDeleted() override;

 protected:
  // BaseNodeModel
  virtual void OnFetchRequested(
      const NodeFetchStatus& requested_status) override;

  const scada::Node* node_ = nullptr;
};
