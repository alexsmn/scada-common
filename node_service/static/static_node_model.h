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
  virtual scada::node GetScadaNode() const override;

 private:
  StaticNodeService& service_;

  // All references are moved to the `references` field. The node state has:
  // * empty `type_definition_id` field;
  // * empty `parent_id` and `reference_type_id` fields;
  // * empty `properties`.
  //
  // See `StaticNodeService::Add`.
  const scada::NodeState node_state_;
};
