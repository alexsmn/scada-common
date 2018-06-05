#include "common/remote_node_model.h"

#include "base/logger.h"
#include "base/strings/sys_string_conversions.h"
#include "common/node_id_util.h"
#include "common/node_observer.h"
#include "common/node_util.h"
#include "common/remote_node_service.h"
#include "core/attribute_service.h"
#include "core/monitored_item.h"
#include "core/standard_node_ids.h"

RemoteNodeModel::RemoteNodeModel(RemoteNodeService& service,
                                 scada::NodeId node_id)
    : BaseNodeModel{std::move(node_id)}, service_{service} {}

void RemoteNodeModel::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    OnNodeDeleted();

    for (auto& o : observers_)
      o.OnModelChanged(event);
  }

  /*if (event.verb & (scada::ModelChangeEvent::ReferenceAdded |
                    scada::ModelChangeEvent::ReferenceDeleted)) {
    service_.OnFetchNode(node_id_, pending_status_);
  }*/
}

void RemoteNodeModel::OnNodeSemanticChanged() {
  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id_);
}

void RemoteNodeModel::OnNodeFetched(scada::NodeState&& node_state) {
  // |super_type_id| is not used.
  assert(node_state.super_type_id.is_null());
  assert(scada::IsTypeDefinition(node_state.node_class) ||
         !node_state.type_definition_id.is_null());
  assert(!scada::IsTypeDefinition(node_state.node_class) ||
         node_state.node_id == scada::id::References ||
         node_state.node_id == scada::id::BaseDataType ||
         node_state.node_id == scada::id::BaseObjectType ||
         node_state.node_id == scada::id::BaseVariableType ||
         (node_state.reference_type_id == scada::id::HasSubtype &&
          !node_state.parent_id.is_null()));

  parent_reference_.reference_type =
      service_.GetNodeImpl(node_state.reference_type_id);
  parent_reference_.target = service_.GetNodeImpl(node_state.parent_id);
  parent_reference_.forward = false;
  if (parent_reference_.target) {
    parent_reference_.target->references_.push_back(
        {parent_reference_.reference_type, shared_from_this(), true});
  }
  //  assert(!parent_ || parent_->GetFetchStatus().node_fetched);

  node_class_ = node_state.node_class;
  attributes_ = std::move(node_state.attributes);
  data_type_ = service_.GetNodeImpl(node_state.attributes.data_type);

  //  assert(!data_type_ || data_type_->GetFetchStatus().node_fetched);

  for (auto& reference : node_state.references) {
    AddReference({service_.GetNodeImpl(reference.reference_type_id),
                  service_.GetNodeImpl(reference.node_id), reference.forward});
  }

  if (!node_state.type_definition_id.is_null())
    type_definition_ = service_.GetNodeImpl(node_state.type_definition_id);
  if (type_definition_) {
    type_definition_->references_.push_back(
        {service_.GetNodeImpl(scada::id::HasTypeDefinition), shared_from_this(),
         false});
  }
  //  assert(!type_definition_ ||
  //  type_definition_->GetFetchStatus().node_fetched);

  if (node_state.reference_type_id == scada::id::HasSubtype)
    supertype_ = parent_reference_.target;

  {
    auto type_definition_id =
        type_definition_ ? type_definition_->node_id_ : scada::NodeId{};
    scada::ModelChangeEvent event{
        node_id_, type_definition_id,
        scada::ModelChangeEvent::ReferenceAdded |
            scada::ModelChangeEvent::ReferenceDeleted};
    for (auto& obs : observers_)
      obs.OnModelChanged(event);
    service_.NotifyModelChanged(event);
  }

  {
    for (auto& obs : observers_)
      obs.OnNodeSemanticChanged(node_id_);
    service_.NotifySemanticsChanged(node_id_);
  }
}

void RemoteNodeModel::OnChildrenFetched(const ReferenceMap& references) {
  child_references_.clear();
  for (auto& p : references) {
    for (auto& [target_id, reference_type_id] : p.second) {
      child_references_.push_back({service_.GetNodeImpl(reference_type_id),
                                   service_.GetNodeImpl(target_id), true});
    }
  }

  auto fetch_status = fetch_status_;
  fetch_status.children_fetched = true;
  SetFetchStatus(status_, fetch_status);

  {
    auto type_definition_id =
        type_definition_ ? type_definition_->node_id_ : scada::NodeId{};
    scada::ModelChangeEvent event{
        node_id_, type_definition_id,
        scada::ModelChangeEvent::ReferenceAdded |
            scada::ModelChangeEvent::ReferenceDeleted};
    for (auto& obs : observers_)
      obs.OnModelChanged(event);
    service_.NotifyModelChanged(event);
  }
}

NodeRef RemoteNodeModel::GetAggregateDeclaration(
    const scada::NodeId& aggregate_declaration_id) const {
  if (!fetch_status_.node_fetched || !status_)
    return nullptr;

  assert(node_class_.has_value());
  auto* type_definition =
      scada::IsTypeDefinition(*node_class_) ? this : type_definition_.get();
  for (; type_definition; type_definition = type_definition->supertype_.get()) {
    assert(type_definition->fetch_status_.node_fetched);
    const auto& declarations = type_definition->references_;
    auto i = std::find_if(
        declarations.begin(), declarations.end(),
        [&aggregate_declaration_id](const NodeModelImplReference& reference) {
          return IsSubtypeOf(reference.reference_type, scada::id::Aggregates) &&
                 reference.target->node_id_ == aggregate_declaration_id;
        });
    if (i != declarations.end())
      return i->target;
  }

  return nullptr;
}

NodeRef RemoteNodeModel::GetAggregate(
    const scada::NodeId& aggregate_declaration_id) const {
  auto aggregate_declaration =
      GetAggregateDeclaration(aggregate_declaration_id);
  if (!aggregate_declaration)
    return nullptr;

  assert(node_class_.has_value());
  if (scada::IsTypeDefinition(node_class_.value()))
    return aggregate_declaration;
  else
    return GetAggregate(aggregate_declaration.browse_name());
}

NodeRef RemoteNodeModel::GetAggregate(
    const scada::QualifiedName& aggregate_name) const {
  if (!fetch_status_.node_fetched)
    return nullptr;

  auto i = std::find_if(
      references_.begin(), references_.end(),
      [aggregate_name](const NodeModelImplReference& reference) {
        assert(reference.reference_type->fetch_status_.node_fetched);
        assert(reference.target->fetch_status_.node_fetched);
        return IsSubtypeOf(reference.reference_type, scada::id::Aggregates) &&
               reference.target->attributes_.browse_name == aggregate_name;
      });
  return i == references_.end() ? nullptr : i->target;
}

NodeRef RemoteNodeModel::GetTarget(const scada::NodeId& reference_type_id,
                                   bool forward) const {
  if (!fetch_status_.node_fetched)
    return nullptr;

  if (forward) {
    if (reference_type_id == scada::id::HasTypeDefinition)
      return type_definition_;

    for (auto& ref : references_) {
      assert(ref.reference_type->fetch_status_.node_fetched);
      if (ref.forward == forward &&
          IsSubtypeOf(ref.reference_type, reference_type_id))
        return ref.target;
    }

    for (auto& ref : child_references_) {
      assert(ref.reference_type->fetch_status_.node_fetched);
      if (ref.forward == forward &&
          IsSubtypeOf(ref.reference_type, reference_type_id))
        return ref.target;
    }

  } else {
    if (reference_type_id == scada::id::HasSubtype)
      return supertype_;

    assert(reference_type_id == scada::id::HierarchicalReferences);
    return parent_reference_.target;
  }

  return nullptr;
}

std::vector<NodeRef> RemoteNodeModel::GetTargets(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef> result;
  if (!fetch_status_.node_fetched)
    return result;

  for (auto& ref : references_) {
    assert(ref.reference_type->fetch_status_.node_fetched);
    assert(ref.target->fetch_status_.node_fetched);
    if (ref.forward == forward &&
        IsSubtypeOf(ref.reference_type, reference_type_id))
      result.emplace_back(ref.target);
  }

  for (auto& ref : child_references_) {
    assert(ref.reference_type->fetch_status_.node_fetched);
    assert(ref.target->fetch_status_.node_fetched);
    if (ref.forward == forward &&
        IsSubtypeOf(ref.reference_type, reference_type_id))
      result.emplace_back(ref.target);
  }

  return result;
}

NodeRef::Reference RemoteNodeModel::GetReference(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef::Reference> result;
  if (!fetch_status_.node_fetched)
    return {};

  // TODO: Optimize.
  if (!forward && reference_type_id == scada::id::HasSubtype)
    return {service_.GetNodeImpl(scada::id::HasSubtype), supertype_, false};

  if (!forward && reference_type_id == scada::id::HierarchicalReferences) {
    return {parent_reference_.reference_type, parent_reference_.target,
            parent_reference_.forward};
  }

  for (auto& ref : references_) {
    assert(ref.reference_type->fetch_status_.node_fetched);
    assert(ref.target->fetch_status_.node_fetched);
    if (ref.forward == forward &&
        IsSubtypeOf(ref.reference_type, reference_type_id))
      return {ref.reference_type, ref.target, ref.forward};
  }

  for (auto& ref : child_references_) {
    assert(ref.reference_type->fetch_status_.node_fetched);
    assert(ref.target->fetch_status_.node_fetched);
    if (ref.forward == forward &&
        IsSubtypeOf(ref.reference_type, reference_type_id))
      return {ref.reference_type, ref.target, ref.forward};
  }

  return {};
}

std::vector<NodeRef::Reference> RemoteNodeModel::GetReferences(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef::Reference> result;
  if (!fetch_status_.node_fetched)
    return result;

  for (auto& ref : references_) {
    assert(ref.reference_type->fetch_status_.node_fetched);
    assert(ref.target->fetch_status_.node_fetched);
    if (ref.forward == forward &&
        IsSubtypeOf(ref.reference_type, reference_type_id))
      result.push_back({ref.reference_type, ref.target, ref.forward});
  }

  for (auto& ref : child_references_) {
    assert(ref.reference_type->fetch_status_.node_fetched);
    assert(ref.target->fetch_status_.node_fetched);
    if (ref.forward == forward &&
        IsSubtypeOf(ref.reference_type, reference_type_id))
      result.push_back({ref.reference_type, ref.target, ref.forward});
  }

  return result;
}

scada::Variant RemoteNodeModel::GetAttribute(
    scada::AttributeId attribute_id) const {
  switch (attribute_id) {
    case scada::AttributeId::NodeId:
      return node_id_;

    case scada::AttributeId::NodeClass:
      return node_class_.has_value()
                 ? scada::Variant{static_cast<int>(*node_class_)}
                 : scada::Variant{};

    case scada::AttributeId::BrowseName:
      if (!fetch_status_.node_fetched || !status_)
        return scada::QualifiedName{NodeIdToScadaString(node_id_)};
      assert(!attributes_.browse_name.empty());
      return attributes_.browse_name;

    case scada::AttributeId::DisplayName:
      if (!fetch_status_.node_fetched)
        return scada::ToLocalizedText(NodeIdToScadaString(node_id_));
      if (!status_)
        return scada::ToLocalizedText(ToString16(status_));
      else if (!attributes_.display_name.empty())
        return attributes_.display_name;
      else
        return scada::ToLocalizedText(attributes_.browse_name.name());

    case scada::AttributeId::Value:
      return attributes_.value;

    default:
      assert(false);
      return {};
  }
}

NodeRef RemoteNodeModel::GetDataType() const {
  return data_type_;
}

void RemoteNodeModel::AddReference(const NodeModelImplReference& reference) {
  //  assert(reference.reference_type->GetFetchStatus().node_fetched);
  //  assert(reference.target->GetFetchStatus().node_fetched);

  if (reference.forward) {
    //    if (IsSubtypeOf(reference.reference_type,
    //    scada::id::HasTypeDefinition))
    //      type_definition_ = reference.target;
    references_.push_back(reference);

    reference.target->references_.push_back(
        {reference.reference_type, shared_from_this(), !reference.forward});

  } else {
    //    if (IsSubtypeOf(reference.reference_type, scada::id::HasSubtype))
    //      supertype_ = reference.target;
  }
}

void RemoteNodeModel::SetError(const scada::Status& status) {
  assert(status_.good());

  status_ = status;
}

void RemoteNodeModel::OnFetchRequested(
    const NodeFetchStatus& requested_status) {
  service_.OnFetchNode(node_id_, requested_status);
}

std::unique_ptr<scada::MonitoredItem> RemoteNodeModel::CreateMonitoredItem(
    scada::AttributeId attribute_id) const {
  return nullptr;
}

void RemoteNodeModel::Call(const scada::NodeId& method_id,
                           const std::vector<scada::Variant>& arguments,
                           const scada::StatusCallback& callback) const {
  callback(scada::StatusCode::Bad_Disconnected);
}
