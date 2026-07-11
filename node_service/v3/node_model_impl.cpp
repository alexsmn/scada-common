#include "node_service/v3/node_model_impl.h"

#include "base/check.h"
#include "model/node_id_util.h"
#include "node_service/node_util.h"
#include "node_service/v3/node_service_impl.h"

#include "base/awaitable.h"

namespace v3 {

// NodeModelImpl

NodeModelImpl::NodeModelImpl(NodeServiceImpl& service,
                             std::shared_ptr<NodeModelRegistry> registry,
                             scada::NodeId node_id)
    : service_{service},
      registry_{std::move(registry)},
      node_id_{std::move(node_id)} {}

NodeModelImpl::~NodeModelImpl() {
  // Unregister through the shared registry (not the service) so a NodeRef
  // released after service teardown stays safe. Skip if the entry was
  // already replaced by a fresh model for the same node id.
  auto i = registry_->nodes.find(node_id_);
  if (i != registry_->nodes.end() && i->second.expired())
    registry_->nodes.erase(i);
}

void NodeModelImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    OnNodeDeleted();

    // A deleted node no longer pins its fetched subtree; holders of
    // previously published snapshots keep their frozen copies.
    child_models_.clear();
    child_references_.clear();

    signals_.model_changed(event);
  }
}

void NodeModelImpl::OnNodeSemanticChanged() {
  signals_.node_semantic_changed(node_id_);
}

void NodeModelImpl::OnFetched(const scada::NodeState& node_state) {
  ++callback_lock_count_;

  node_state_ = std::move(node_state);

  if (!node_state_.parent_id.is_null()) {
    node_state_.references.push_back(scada::ReferenceDescription{
        node_state_.reference_type_id, false, node_state_.parent_id});
  }

  if (!node_state_.supertype_id.is_null()) {
    node_state_.references.push_back(scada::ReferenceDescription{
        scada::id::HasSubtype, false, node_state_.supertype_id});
  }

  // Push inverse references only into models that are already resident;
  // materializing the whole reference neighborhood here would grow the node
  // graph unboundedly as a side effect of fetching a single node.
  for (const auto& ref : node_state_.references) {
    if (ref.node_id != node_id_) {
      if (auto target = service_.FindNodeModel(ref.node_id)) {
        target->node_state_.references.push_back(scada::ReferenceDescription{
            ref.reference_type_id, !ref.forward, node_id_});
        // Each affected neighbor publishes its own snapshot. Unfetched
        // neighbors publish nothing: their working state is replaced
        // wholesale when their own fetch completes.
        if (target->fetch_status_.node_fetched)
          target->NotifyStateChanged();
      }
    }
  }

  SetFetchStatus(scada::StatusCode::Good, NodeFetchStatus::NodeOnly());

  NotifyStateChanged();
  NotifyModelChanged();
  NotifySemanticChanged();
}

void NodeModelImpl::OnFetchCompleted() {
  base::Check(callback_lock_count_ > 0);
  --callback_lock_count_;

  // Publish the snapshot before the change notifications so consumers that
  // cache snapshots from OnNodeStateChanged have fresh data by the time
  // OnModelChanged/OnNodeSemanticChanged fire.
  if (pending_state_changed_)
    NotifyStateChanged();

  if (pending_model_changed_)
    NotifyModelChanged();

  if (pending_semantic_changed_)
    NotifySemanticChanged();

  NotifyCallbacks();
}

void NodeModelImpl::OnFetchError(scada::Status&& status) {
  SetFetchStatus(std::move(status), NodeFetchStatus::Max());
}

void NodeModelImpl::OnChildrenFetched(
    scada::ReferenceDescriptions&& references) {
  auto shared_references =
      std::make_shared<scada::ReferenceDescriptions>(std::move(references));

  reference_request_ = std::make_shared<bool>(false);
  std::weak_ptr<bool> reference_request = reference_request_;
  CoSpawn(service_.executor_,
          [reference_request, this, &service = service_,
           shared_references]() -> Awaitable<void> {
            // Fetch and pin every child target and its reference type. Until
            // the pins are handed to the parent below, they keep the fetched
            // nodes resident across the coroutine suspension points.
            std::vector<NodeRef> fetched_nodes;
            fetched_nodes.reserve(shared_references->size() * 2);
            for (const auto& ref : *shared_references) {
              auto target = service.GetNode(ref.node_id);
              co_await target.Fetch(NodeFetchStatus::NodeOnly());
              auto reference_type = service.GetNode(ref.reference_type_id);
              co_await reference_type.Fetch(NodeFetchStatus::NodeOnly());
              fetched_nodes.push_back(std::move(target));
              fetched_nodes.push_back(std::move(reference_type));
            }

            // |this| may only be touched past this point: |reference_request|
            // guards against both a superseding request and the model having
            // been released mid-flight (the flag dies with the model).
            if (!reference_request.lock())
              co_return;

            child_references_ = *shared_references;
            // The parent pins its fetched subtree; releasing the last NodeRef
            // to the parent releases the children too.
            child_models_ = std::move(fetched_nodes);

            auto fetch_status = fetch_status_;
            fetch_status.children_fetched = true;
            SetFetchStatus(status_, fetch_status);

            NotifyStateChanged();
            NotifyModelChanged();
          });
}

NodeRef NodeModelImpl::GetAggregateDeclaration(
    const scada::NodeId& aggregate_declaration_id) const {
  if (!fetch_status_.node_fetched || !status_)
    return nullptr;

  NodeRef type_definition =
      scada::IsTypeDefinition(node_state_.node_class)
          ? NodeRef{node_id_, &service_}
          : service_.GetNode(node_state_.type_definition_id);

  // TODO: Optimize.
  for (; type_definition; type_definition = type_definition.supertype()) {
    for (const auto& aggregate :
         type_definition.targets(scada::id::Aggregates)) {
      if (aggregate.node_id() == aggregate_declaration_id)
        return aggregate;
    }
  }

  return nullptr;
}

NodeRef NodeModelImpl::GetAggregate(
    const scada::NodeId& aggregate_declaration_id) const {
  auto aggregate_declaration =
      GetAggregateDeclaration(aggregate_declaration_id);
  if (!aggregate_declaration)
    return nullptr;

  if (scada::IsTypeDefinition(node_state_.node_class))
    return aggregate_declaration;
  else
    return GetChild(aggregate_declaration.browse_name());
}

NodeRef NodeModelImpl::GetChild(const scada::QualifiedName& child_name) const {
  if (!fetch_status_.node_fetched)
    return nullptr;

  // TODO: Optimize.
  for (const auto& child :
       GetTargets(scada::id::HierarchicalReferences, true)) {
    if (child.browse_name() == child_name)
      return child;
  }
  return nullptr;
}

NodeRef NodeModelImpl::GetTarget(const scada::NodeId& reference_type_id,
                                 bool forward) const {
  // An optimization for type definition.
  if (forward && reference_type_id == scada::id::HasTypeDefinition)
    return service_.GetNode(node_state_.type_definition_id);

  // Required to break the cycle. And an optimization for supertype.
  if (!forward && reference_type_id == scada::id::HasSubtype)
    return service_.GetNode(node_state_.supertype_id);

  // An optimization for parent.
  if (!forward && reference_type_id == scada::id::HierarchicalReferences) {
    if (!node_state_.parent_id.is_null())
      return service_.GetNode(node_state_.parent_id);

    if (!node_state_.supertype_id.is_null())
      return service_.GetNode(node_state_.supertype_id);
  }

  return GetReference(reference_type_id, forward, {}).target;
}

std::vector<NodeRef> NodeModelImpl::GetTargets(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef> result;

  const auto& refs = GetReferences(reference_type_id, forward);
  result.reserve(refs.size());
  for (const auto& ref : refs)
    result.emplace_back(ref.target);

  return result;
}

NodeRef::Reference NodeModelImpl::GetReference(
    const scada::NodeId& reference_type_id,
    bool forward,
    const scada::NodeId& node_id) const {
  // TODO: Optimize.
  if (!forward && reference_type_id == scada::id::HierarchicalReferences &&
      node_id.is_null() && !node_state_.parent_id.is_null()) {
    return NodeRef::Reference{service_.GetNode(node_state_.reference_type_id),
                              service_.GetNode(node_state_.parent_id), false};
  }

  auto refs = GetReferences(reference_type_id, forward);
  if (refs.empty())
    return {};

  if (node_id.is_null())
    return refs.front();

  for (auto& ref : refs) {
    if (ref.target.node_id() == node_id)
      return ref;
  }
  return {};
}

std::vector<NodeRef::Reference> NodeModelImpl::GetReferences(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef::Reference> result;

  for (auto& ref : node_state_.references) {
    if (ref.forward == forward) {
      // The reference type may not be fetched yet (remote data); the
      // subtype test below then simply fails to match.
      auto reference_type = service_.GetNode(ref.reference_type_id);
      if (IsSubtypeOf(reference_type, reference_type_id)) {
        result.push_back(
            {reference_type, service_.GetNode(ref.node_id), ref.forward});
      }
    }
  }

  for (auto& ref : child_references_) {
    if (ref.forward == forward) {
      // The reference type may not be fetched yet (remote data); the
      // subtype test below then simply fails to match.
      auto reference_type = service_.GetNode(ref.reference_type_id);
      if (IsSubtypeOf(reference_type, reference_type_id)) {
        result.push_back(
            {reference_type, service_.GetNode(ref.node_id), ref.forward});
      }
    }
  }

  return result;
}

scada::Variant NodeModelImpl::GetAttribute(
    scada::AttributeId attribute_id) const {
  switch (attribute_id) {
    case scada::AttributeId::NodeId:
      return node_id_;

    case scada::AttributeId::NodeClass:
      if (!fetch_status_.node_fetched || !status_)
        return {};
      return static_cast<scada::Int32>(node_state_.node_class);

    case scada::AttributeId::BrowseName:
      if (!fetch_status_.node_fetched || !status_)
        return scada::QualifiedName{NodeIdToScadaString(node_id_)};
      // The browse name was fetched from a (possibly remote) server and may
      // be missing in malformed responses.
      return node_state_.attributes.browse_name;

    case scada::AttributeId::DisplayName:
      if (!fetch_status_.node_fetched)
        return scada::ToLocalizedText(NodeIdToScadaString(node_id_));
      if (!status_)
        return scada::ToLocalizedText(ToString16(status_));
      else if (!node_state_.attributes.display_name.empty())
        return node_state_.attributes.display_name;
      else
        return scada::ToLocalizedText(
            node_state_.attributes.browse_name.name());

    case scada::AttributeId::Value:
      return node_state_.attributes.value.value_or(scada::Variant{});

    default:
      // Unsupported attributes yield an empty variant.
      return {};
  }
}

NodeRef NodeModelImpl::GetDataType() const {
  return service_.GetNode(node_state_.attributes.data_type);
}

void NodeModelImpl::SetError(const scada::Status& status) {
  base::Check(status_.good());

  status_ = status;
}

void NodeModelImpl::OnFetchRequested(const NodeFetchStatus& requested_status) {
  service_.OnFetchNode(node_id_, requested_status);
}

void NodeModelImpl::NotifyModelChanged() {
  if (callback_lock_count_ != 0) {
    pending_model_changed_ = true;
    return;
  }

  pending_model_changed_ = false;

  scada::ModelChangeEvent event{node_id_, node_state_.type_definition_id,
                                scada::ModelChangeEvent::ReferenceAdded |
                                    scada::ModelChangeEvent::ReferenceDeleted};

  signals_.model_changed(event);

  service_.NotifyModelChanged(event);
}

void NodeModelImpl::NotifySemanticChanged() {
  if (callback_lock_count_ != 0) {
    pending_semantic_changed_ = true;
    return;
  }

  pending_semantic_changed_ = false;

  signals_.node_semantic_changed(node_id_);

  service_.NotifySemanticsChanged(node_id_);
}

void NodeModelImpl::NotifyStateChanged() {
  if (callback_lock_count_ != 0) {
    pending_state_changed_ = true;
    return;
  }

  pending_state_changed_ = false;

  // Build a self-contained snapshot: the working state plus the fetched
  // child references (kept separately in |child_references_|). Published
  // snapshots are never mutated — holders keep a frozen, consistent view.
  auto state = node_state_;
  state.references.insert(state.references.end(), child_references_.begin(),
                          child_references_.end());

  const NodeStateChangedEvent event{
      node_id_, std::make_shared<const scada::NodeState>(std::move(state)),
      fetch_status_};

  signals_.node_state_changed(event);

  service_.NotifyNodeStateChanged(event);
}

scada::node NodeModelImpl::GetScadaNode() const {
  return {};
}

}  // namespace v3
