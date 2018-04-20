#include "common/address_space/address_space_node_model.h"

#include "common/address_space/property_node_model.h"
#include "common/node_id_util.h"
#include "common/node_observer.h"
#include "common/node_service.h"

AddressSpaceNodeModel::AddressSpaceNodeModel(
    AddressSpaceNodeModelDelegate& delegate,
    scada::NodeId node_id)
    : BaseNodeModel{std::move(node_id)}, delegate_{delegate} {}

AddressSpaceNodeModel::~AddressSpaceNodeModel() {
  delegate_.OnNodeModelDeleted(node_id_);
}

void AddressSpaceNodeModel::OnModelChanged(
    const scada::ModelChangeEvent& event) {
  for (auto& o : observers_)
    o.OnModelChanged(event);

  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    OnNodeDeleted();

  } else if (event.verb & scada::ModelChangeEvent::NodeAdded) {
    if (!fetch_status_.empty()) {
      auto fetch_status = fetch_status_;
      SetFetchStatus(node_, scada::StatusCode::Good, NodeFetchStatus());
      delegate_.OnNodeModelFetchRequested(node_id_, fetch_status);
    }
  }
}

void AddressSpaceNodeModel::OnNodeSemanticChanged() {
  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id_);
}

scada::Variant AddressSpaceNodeModel::GetAttribute(
    scada::AttributeId attribute_id) const {
  if (attribute_id == scada::AttributeId::NodeId)
    return node_id_;

  if (node_) {
    switch (attribute_id) {
      case scada::AttributeId::NodeClass:
        return static_cast<int>(node_->GetNodeClass());
      case scada::AttributeId::BrowseName:
        return node_->GetBrowseName();
      case scada::AttributeId::DisplayName:
        return node_->GetDisplayName();
      default:
        return {};
    }

  } else {
    switch (attribute_id) {
      case scada::AttributeId::DisplayName:
        return scada::ToLocalizedText(NodeIdToScadaString(node_id_));
      default:
        return {};
    }
  }
}

std::vector<NodeRef::Reference> AddressSpaceNodeModel::GetReferences(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef::Reference> result;

  if (!node_)
    return result;

  auto& refs =
      forward ? node_->forward_references() : node_->inverse_references();
  for (auto& r : refs) {
    if (scada::IsSubtypeOf(*r.type, scada::id::NonHierarchicalReferences))
      result.push_back({delegate_.GetRemoteNode(r.type),
                        delegate_.GetRemoteNode(r.node), forward});
  }

  return result;
}

NodeRef AddressSpaceNodeModel::GetDataType() const {
  auto* variable = scada::AsVariable(node_);
  auto* data_type = variable ? &variable->GetDataType() : nullptr;
  return delegate_.GetRemoteNode(data_type);
}

NodeRef AddressSpaceNodeModel::GetAggregate(
    const scada::NodeId& declaration_id) const {
  if (!node_)
    return nullptr;

  auto* type_definition = scada::IsTypeDefinition(node_->GetNodeClass())
                              ? scada::AsTypeDefinition(node_)
                              : scada::GetTypeDefinition(*node_);
  if (!type_definition)
    return nullptr;

  auto* declaration =
      scada::GetAggregateDeclaration(*type_definition, declaration_id);
  if (!declaration)
    return nullptr;

  if (scada::IsTypeDefinition(node_->GetNodeClass())) {
    return delegate_.GetRemoteNode(declaration);
  } else if (scada::IsInstanceOf(declaration, scada::id::PropertyType)) {
    return std::make_shared<AddressSpacePropertyModel>(
        shared_from_this(), delegate_.GetRemoteNode(declaration).model());
  } else {
    return GetAggregate(declaration->GetBrowseName());
  }
}

NodeRef AddressSpaceNodeModel::GetAggregate(
    const scada::QualifiedName& aggregate_name) const {
  if (!node_)
    return nullptr;
  for (auto* node : scada::GetAggregates(*node_)) {
    if (node->GetBrowseName() == aggregate_name)
      return delegate_.GetRemoteNode(node);
  }
  return nullptr;
}

NodeRef AddressSpaceNodeModel::GetTarget(const scada::NodeId& reference_type_id,
                                         bool forward) const {
  if (!node_)
    return nullptr;

  auto& refs =
      forward ? node_->forward_references() : node_->inverse_references();
  for (auto& r : refs) {
    if (scada::IsSubtypeOf(*r.type, reference_type_id))
      return delegate_.GetRemoteNode(r.node);
  }

  return nullptr;
}

std::vector<NodeRef> AddressSpaceNodeModel::GetTargets(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef> result;

  if (!node_)
    return result;

  auto& refs =
      forward ? node_->forward_references() : node_->inverse_references();
  for (auto& r : refs) {
    if (scada::IsSubtypeOf(*r.type, reference_type_id))
      result.emplace_back(delegate_.GetRemoteNode(r.node));
  }

  return result;
}

NodeRef::Reference AddressSpaceNodeModel::GetReference(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  if (!node_)
    return {};

  auto reference = scada::FindReference(*node_, reference_type_id, forward);
  return {delegate_.GetRemoteNode(reference.type),
          delegate_.GetRemoteNode(reference.node), forward};
}

void AddressSpaceNodeModel::SetFetchStatus(
    const scada::Node* node,
    const scada::Status& status,
    const NodeFetchStatus& fetch_status) {
  node_ = node;

  BaseNodeModel::SetFetchStatus(status, fetch_status);

  const scada::ModelChangeEvent event{
      node_id_, node_ ? scada::GetTypeDefinitionId(*node_) : scada::NodeId{},
      scada::ModelChangeEvent::ReferenceAdded |
          scada::ModelChangeEvent::ReferenceDeleted};
  for (auto& o : observers_) {
    o.OnModelChanged(event);
    o.OnNodeSemanticChanged(node_id_);
  }
}

void AddressSpaceNodeModel::OnFetchRequested(
    const NodeFetchStatus& requested_status) {
  delegate_.OnNodeModelFetchRequested(node_id_, requested_status);
}

void AddressSpaceNodeModel::OnNodeDeleted() {
  node_ = nullptr;

  BaseNodeModel::OnNodeDeleted();
}

scada::Variant AddressSpaceNodeModel::GetPropertyAttribute(
    const NodeModel& property_declaration,
    scada::AttributeId attribute_id) const {
  switch (attribute_id) {
    case scada::AttributeId::NodeId:
      return MakeNestedNodeId(
          GetAttribute(scada::AttributeId::NodeId).as_node_id(),
          property_declaration.GetAttribute(scada::AttributeId::BrowseName)
              .get<scada::QualifiedName>()
              .name());

    case scada::AttributeId::Value:
      if (!node_)
        return {};
      return scada::GetPropertyValue(
          *node_, property_declaration.GetAttribute(scada::AttributeId::NodeId)
                      .as_node_id());

    default:
      return property_declaration.GetAttribute(attribute_id);
  }
}
