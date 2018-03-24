#pragma once

#include "common/base_node_model.h"
#include "core/node_utils.h"
#include "core/type_definition.h"
#include "core/variable.h"

namespace scada {
struct ModelChangeEvent;
}

class AddressSpaceNodeModel;

class AddressSpaceNodeModelDelegate {
 public:
  virtual ~AddressSpaceNodeModelDelegate() {}

  // If |node| is nullptr, empty node model is returned.
  virtual NodeRef GetRemoteNode(const scada::Node* node) = 0;

  virtual void OnRemoteNodeModelDeleted(const scada::NodeId& node_id) = 0;
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

  void OnNodeFetchStatusChanged(const scada::Node* node,
                                const NodeFetchStatus& status);

  scada::Variant GetPropertyAttribute(const NodeModel& property_declaration,
                                      scada::AttributeId attribute_id) const;

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

 protected:
  // BaseNodeModel
  virtual void OnNodeDeleted() override;

  AddressSpaceNodeModelDelegate& delegate_;
  const scada::Node* node_ = nullptr;
};
