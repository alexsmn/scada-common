#pragma once

#include "common/node_state.h"
#include "node_service/node_ref.h"

#include <vector>

class StaticNodeService;

// One node of a StaticNodeService, holding its fully materialized state.
//
// Static nodes carry no fetch machinery: everything is present the moment the
// node is added, so the owning service answers status and fetch queries itself
// (always Good / fully fetched). This is a plain state holder owned by the
// service, not a polymorphic node abstraction.
class StaticNodeModel {
 public:
  StaticNodeModel(StaticNodeService& service, scada::NodeState node_state);

  scada::Variant GetAttribute(scada::AttributeId attribute_id) const;
  NodeRef GetDataType() const;
  NodeRef::Reference GetReference(const scada::NodeId& reference_type_id,
                                  bool forward,
                                  const scada::NodeId& node_id) const;
  std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const;
  NodeRef GetTarget(const scada::NodeId& reference_type_id,
                    bool forward) const;
  std::vector<NodeRef> GetTargets(const scada::NodeId& reference_type_id,
                                  bool forward) const;
  NodeRef GetChild(const scada::QualifiedName& child_name) const;
  scada::node GetScadaNode() const;

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
