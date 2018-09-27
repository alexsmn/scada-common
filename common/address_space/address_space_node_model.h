#pragma once

#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "common/base_node_model.h"

namespace scada {
struct ModelChangeEvent;
}

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

  virtual std::unique_ptr<scada::MonitoredItem> OnNodeModelCreateMonitoredItem(
      const scada::ReadValueId& read_value_id) = 0;

  virtual void OnNodeModelWrite(const scada::NodeId& node_id,
                                double value,
                                const scada::NodeId& user_id,
                                const scada::WriteFlags& flags,
                                const scada::StatusCallback& callback) = 0;

  virtual void OnNodeModelCall(const scada::NodeId& node_id,
                               const scada::NodeId& method_id,
                               const std::vector<scada::Variant>& arguments,
                               const scada::NodeId& user_id,
                               const scada::StatusCallback& callback) = 0;
};

class AddressSpaceNodeModel final
    : public BaseNodeModel,
      public std::enable_shared_from_this<AddressSpaceNodeModel> {
 public:
  AddressSpaceNodeModel(AddressSpaceNodeModelDelegate& delegate,
                        scada::NodeId node_id);
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
      bool forward) const override;
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
  virtual NodeRef GetAggregate(
      const scada::QualifiedName& aggregate_name) const override;
  virtual std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(
      scada::AttributeId attribute_id) const override;
  virtual void Write(const scada::NodeId& node_id,
                     double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const scada::StatusCallback& callback) const override;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::StatusCallback& callback) const override;

 protected:
  // BaseNodeModel
  virtual void OnFetchRequested(
      const NodeFetchStatus& requested_status) override;
  virtual void OnNodeDeleted() override;

  AddressSpaceNodeModelDelegate& delegate_;
  const scada::Node* node_ = nullptr;
};
