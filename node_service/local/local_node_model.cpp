#include "node_service/local/local_node_model.h"

#include "base/utf_convert.h"
#include "scada/localized_text.h"

LocalNodeModel::LocalNodeModel(Data data) : data_{std::move(data)} {
  fetch_status_ = NodeFetchStatus::Max();
}

scada::Variant LocalNodeModel::GetAttribute(
    scada::AttributeId attribute_id) const {
  switch (attribute_id) {
    case scada::AttributeId::NodeId:
      return scada::Variant{data_.node_id};
    case scada::AttributeId::DisplayName:
      return scada::Variant{
          scada::LocalizedText{UtfConvert<char16_t>(data_.display_name)}};
    case scada::AttributeId::NodeClass:
      return scada::Variant{static_cast<scada::Int32>(data_.node_class)};
    default:
      return {};
  }
}

NodeRef LocalNodeModel::GetDataType() const {
  return {};
}

NodeRef::Reference LocalNodeModel::GetReference(
    const scada::NodeId& /*reference_type_id*/,
    bool /*forward*/,
    const scada::NodeId& /*node_id*/) const {
  return {};
}

std::vector<NodeRef::Reference> LocalNodeModel::GetReferences(
    const scada::NodeId& /*reference_type_id*/,
    bool /*forward*/) const {
  return {};
}

NodeRef LocalNodeModel::GetTarget(const scada::NodeId& /*reference_type_id*/,
                                  bool /*forward*/) const {
  return {};
}

std::vector<NodeRef> LocalNodeModel::GetTargets(
    const scada::NodeId& /*reference_type_id*/,
    bool /*forward*/) const {
  return {};
}

NodeRef LocalNodeModel::GetAggregate(
    const scada::NodeId& /*aggregate_declaration_id*/) const {
  return {};
}

NodeRef LocalNodeModel::GetChild(
    const scada::QualifiedName& /*child_name*/) const {
  return {};
}

scada::node LocalNodeModel::GetScadaNode() const {
  return {};
}
