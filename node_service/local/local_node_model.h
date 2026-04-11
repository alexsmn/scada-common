#pragma once

#include "node_service/base_node_model.h"

#include <string>

// In-memory NodeModel backed by a small fixed struct. Supports reads for the
// three attributes the client UI routinely queries (`NodeId`, `DisplayName`,
// `NodeClass`) and returns empty for everything else. Companion to
// `LocalNodeService`.
class LocalNodeModel : public BaseNodeModel {
 public:
  struct Data {
    scada::NodeId node_id;
    std::string display_name;
    scada::NodeClass node_class = scada::NodeClass::Object;
  };

  explicit LocalNodeModel(Data data);

  // NodeModel
  scada::Variant GetAttribute(scada::AttributeId attribute_id) const override;
  NodeRef GetDataType() const override;
  NodeRef::Reference GetReference(const scada::NodeId& reference_type_id,
                                  bool forward,
                                  const scada::NodeId& node_id) const override;
  std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override;
  NodeRef GetTarget(const scada::NodeId& reference_type_id,
                    bool forward) const override;
  std::vector<NodeRef> GetTargets(const scada::NodeId& reference_type_id,
                                  bool forward) const override;
  NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const override;
  NodeRef GetChild(const scada::QualifiedName& child_name) const override;
  scada::node GetScadaNode() const override;

 private:
  const Data data_;
};
