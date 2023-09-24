#include "node_service/v3/node_model_impl.h"

#include "model/node_id_util.h"
#include "node_service/node_observer.h"
#include "node_service/node_util.h"
#include "node_service/v3/node_service_impl.h"

namespace v3 {

namespace {

template <class Callback>
struct JoinedRequest {
  explicit JoinedRequest(Callback&& callback)
      : callback{std::forward<Callback>(callback)} {}

  void CountDown() { Update(-1); }
  void Wait(int count) { Update(count); }

  void Update(int delta) {
    int left_count = expected_count += delta;
    if (left_count == 0)
      callback();
  }

  const Callback callback;
  std::atomic<int> expected_count = 0;
};

template <class Callback>
void FetchReferences(NodeService& service,
                     const scada::ReferenceDescriptions& references,
                     Callback&& callback) {
  auto request = std::make_shared<JoinedRequest<Callback>>(
      std::forward<Callback>(callback));

  int count = 0;
  for (auto& ref : references) {
    service.GetNode(ref.node_id)
        .Fetch(NodeFetchStatus::NodeOnly(),
               [request](const NodeRef& node) { request->CountDown(); });
    service.GetNode(ref.reference_type_id)
        .Fetch(NodeFetchStatus::NodeOnly(),
               [request](const NodeRef& node) { request->CountDown(); });
    count += 2;
  }

  request->Wait(count);
}

}  // namespace

// NodeModelImpl

NodeModelImpl::NodeModelImpl(NodeServiceImpl& service, scada::NodeId node_id)
    : service_{service}, node_id_{std::move(node_id)} {}

void NodeModelImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    OnNodeDeleted();

    for (auto& o : observers_)
      o.OnModelChanged(event);
  }
}

void NodeModelImpl::OnNodeSemanticChanged() {
  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id_);
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

  for (const auto& ref : node_state_.references) {
    if (ref.node_id != node_id_) {
      if (auto target = service_.GetNodeModel(ref.node_id)) {
        target->node_state_.references.push_back(scada::ReferenceDescription{
            ref.reference_type_id, !ref.forward, node_id_});
      }
    }
  }

  SetFetchStatus(scada::StatusCode::Good, NodeFetchStatus::NodeOnly());

  NotifyModelChanged();
  NotifySemanticChanged();
}

void NodeModelImpl::OnFetchCompleted() {
  assert(callback_lock_count_ > 0);
  --callback_lock_count_;

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
  FetchReferences(service_, *shared_references,
                  [reference_request, this, shared_references] {
                    if (!reference_request.lock())
                      return;

                    child_references_ = *shared_references;

                    auto fetch_status = fetch_status_;
                    fetch_status.children_fetched = true;
                    SetFetchStatus(status_, fetch_status);

                    NotifyModelChanged();
                  });
}

NodeRef NodeModelImpl::GetAggregateDeclaration(
    const scada::NodeId& aggregate_declaration_id) const {
  if (!fetch_status_.node_fetched || !status_)
    return nullptr;

  NodeRef type_definition =
      scada::IsTypeDefinition(node_state_.node_class)
          ? shared_from_this()
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
      auto reference_type = service_.GetNode(ref.reference_type_id);
      assert(reference_type.fetched());
      if (IsSubtypeOf(reference_type, reference_type_id)) {
        result.push_back(
            {reference_type, service_.GetNode(ref.node_id), ref.forward});
      }
    }
  }

  for (auto& ref : child_references_) {
    if (ref.forward == forward) {
      auto reference_type = service_.GetNode(ref.reference_type_id);
      assert(reference_type.fetched());
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
      assert(!node_state_.attributes.browse_name.empty());
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
      assert(false);
      return {};
  }
}

NodeRef NodeModelImpl::GetDataType() const {
  return service_.GetNode(node_state_.attributes.data_type);
}

void NodeModelImpl::SetError(const scada::Status& status) {
  assert(status_.good());

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

  for (auto& obs : observers_)
    obs.OnModelChanged(event);

  service_.NotifyModelChanged(event);
}

void NodeModelImpl::NotifySemanticChanged() {
  if (callback_lock_count_ != 0) {
    pending_semantic_changed_ = true;
    return;
  }

  pending_semantic_changed_ = false;

  for (auto& obs : observers_)
    obs.OnNodeSemanticChanged(node_id_);

  service_.NotifySemanticsChanged(node_id_);
}

scada::node NodeModelImpl::GetScadaNode() const {
  return {};
}

}  // namespace v3
