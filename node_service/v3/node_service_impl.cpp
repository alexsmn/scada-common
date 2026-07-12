#include "node_service/v3/node_service_impl.h"

#include "base/awaitable.h"
#include "base/check.h"
#include "node_service/node_util.h"
#include "node_service/v3/node_fetcher.h"
#include "node_service/v3/node_model_impl.h"
#include "scada/attribute_service.h"
#include "scada/standard_node_ids.h"

namespace v3 {

namespace {

std::unique_ptr<IViewEventsSubscription> MakeViewEventsSubscription(
    AnyExecutor executor,
    scada::MonitoredItemService& monitored_item_service,
    scada::ViewEvents& events,
    const ViewEventsProvider& provider) {
  if (provider)
    return provider(events);
  return std::make_unique<ViewEventsSubscription>(
      executor, monitored_item_service, events);
}

}  // namespace

NodeServiceImpl::NodeServiceImpl(NodeServiceImplContext&& context)
    : NodeServiceImplContext(std::move(context)),
      view_events_subscription_{
          MakeViewEventsSubscription(executor_,
                                     monitored_item_service_,
                                     *this,
                                     view_events_provider_)} {}

NodeServiceImpl::~NodeServiceImpl() {}

std::shared_ptr<NodeModelImpl> NodeServiceImpl::GetNodeModelForRead(
    const scada::NodeId& node_id) {
  auto node = GetNodeModel(node_id);
  if (node)
    node->StartFetch(NodeFetchStatus::NodeOnly());
  return node;
}

NodeRef NodeServiceImpl::GetNode(const scada::NodeId& node_id) {
  auto node = GetNodeModelForRead(node_id);
  return node ? NodeRef{node_id, this} : nullptr;
}

scada::Status NodeServiceImpl::GetStatus(const scada::NodeId& node_id) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetStatus()
              : scada::Status{scada::StatusCode::Bad_WrongNodeId};
}

NodeFetchStatus NodeServiceImpl::GetFetchStatus(const scada::NodeId& node_id) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetFetchStatus() : NodeFetchStatus{};
}

Awaitable<void> NodeServiceImpl::Fetch(
    const scada::NodeId& node_id,
    const NodeFetchStatus& requested_status) {
  auto node = GetNodeModelForRead(node_id);
  if (node)
    co_await node->Fetch(requested_status);
}

void NodeServiceImpl::StartFetch(const scada::NodeId& node_id,
                                 const NodeFetchStatus& requested_status) {
  if (auto node = GetNodeModel(node_id))
    node->StartFetch(requested_status);
}

scada::Variant NodeServiceImpl::GetAttribute(const scada::NodeId& node_id,
                                             scada::AttributeId attribute_id) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetAttribute(attribute_id) : scada::Variant{};
}

NodeRef NodeServiceImpl::GetDataType(const scada::NodeId& node_id) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetDataType() : nullptr;
}

NodeRef::Reference NodeServiceImpl::GetReference(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward,
    const scada::NodeId& target_id) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetReference(reference_type_id, forward, target_id)
              : NodeRef::Reference{};
}

std::vector<NodeRef::Reference> NodeServiceImpl::GetReferences(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetReferences(reference_type_id, forward)
              : std::vector<NodeRef::Reference>{};
}

NodeRef NodeServiceImpl::GetTarget(const scada::NodeId& node_id,
                                   const scada::NodeId& reference_type_id,
                                   bool forward) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetTarget(reference_type_id, forward) : nullptr;
}

std::vector<NodeRef> NodeServiceImpl::GetTargets(
    const scada::NodeId& node_id,
    const scada::NodeId& reference_type_id,
    bool forward) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetTargets(reference_type_id, forward)
              : std::vector<NodeRef>{};
}

NodeRef NodeServiceImpl::GetAggregate(
    const scada::NodeId& node_id,
    const scada::NodeId& aggregate_declaration_id) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetAggregate(aggregate_declaration_id) : nullptr;
}

NodeRef NodeServiceImpl::GetChild(const scada::NodeId& node_id,
                                  const scada::QualifiedName& child_name) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetChild(child_name) : NodeRef{};
}

scada::node NodeServiceImpl::GetScadaNode(const scada::NodeId& node_id) {
  auto node = GetNodeModelForRead(node_id);
  return node ? node->GetScadaNode() : scada::node{};
}

// Node-scoped subscriptions live in the service's subscription table, not on
// the model. Subscribing no longer materializes or pins the node's model:
// remote change events reach per-node observers through the service's
// On*/Notify* path regardless of residency (see OnModelChanged /
// OnNodeSemanticsChanged), and local model-driven notifications only fire while
// a fetch keeps the model resident anyway.
boost::signals2::scoped_connection NodeServiceImpl::SubscribeModelChanged(
    const scada::NodeId& node_id,
    const ModelChangedCallback& callback) {
  if (node_id.is_null())
    return {};
  return subscription_table_.SubscribeModelChanged(node_id, callback);
}

boost::signals2::scoped_connection
NodeServiceImpl::SubscribeNodeSemanticChanged(
    const scada::NodeId& node_id,
    const NodeSemanticChangedCallback& callback) {
  if (node_id.is_null())
    return {};
  return subscription_table_.SubscribeNodeSemanticChanged(node_id, callback);
}

boost::signals2::scoped_connection NodeServiceImpl::SubscribeNodeFetched(
    const scada::NodeId& node_id,
    const NodeFetchedCallback& callback) {
  if (node_id.is_null())
    return {};
  return subscription_table_.SubscribeNodeFetched(node_id, callback);
}

boost::signals2::scoped_connection NodeServiceImpl::SubscribeNodeStateChanged(
    const scada::NodeId& node_id,
    const NodeStateChangedCallback& callback) {
  if (node_id.is_null())
    return {};
  return subscription_table_.SubscribeNodeStateChanged(node_id, callback);
}

std::shared_ptr<NodeModelImpl> NodeServiceImpl::GetNodeModel(
    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  auto& entry = registry_->nodes[node_id];
  auto node = entry.lock();
  if (!node) {
    node = std::make_shared<NodeModelImpl>(*this, registry_, node_id);
    entry = node;
  }

  TouchKeepAlive(node);
  return node;
}

std::shared_ptr<NodeModelImpl> NodeServiceImpl::FindNodeModel(
    const scada::NodeId& node_id) const {
  auto i = registry_->nodes.find(node_id);
  return i != registry_->nodes.end() ? i->second.lock() : nullptr;
}

void NodeServiceImpl::TouchKeepAlive(std::shared_ptr<NodeModelImpl> model) {
  if (keep_alive_capacity_ == 0)
    return;

  if (auto i = keep_alive_index_.find(model->node_id());
      i != keep_alive_index_.end()) {
    keep_alive_list_.splice(keep_alive_list_.begin(), keep_alive_list_,
                            i->second);
    return;
  }

  // Copy the key out before the insert: the insert std::moves `model`, and the
  // right-hand side is sequenced before the left, so reading model->node_id()
  // in the subscript would touch a moved-from pointer. Keep this copy.
  const scada::NodeId node_id = model->node_id();
  keep_alive_index_[node_id] =
      keep_alive_list_.insert(keep_alive_list_.begin(), std::move(model));

  while (keep_alive_list_.size() > keep_alive_capacity_) {
    keep_alive_index_.erase(keep_alive_list_.back()->node_id());
    // May destroy the model, which unregisters itself from the registry.
    keep_alive_list_.pop_back();
  }
}

void NodeServiceImpl::RemoveFromKeepAlive(const scada::NodeId& node_id) {
  if (auto i = keep_alive_index_.find(node_id); i != keep_alive_index_.end()) {
    auto list_iterator = i->second;
    keep_alive_index_.erase(i);
    // May destroy the model, which unregisters itself from the registry.
    keep_alive_list_.erase(list_iterator);
  }
}

boost::signals2::scoped_connection NodeServiceImpl::SubscribeModelChanged(
    const ModelChangedCallback& callback) const {
  return signals_.model_changed.connect(callback);
}

boost::signals2::scoped_connection
NodeServiceImpl::SubscribeNodeSemanticChanged(
    const NodeSemanticChangedCallback& callback) const {
  return signals_.node_semantic_changed.connect(callback);
}

boost::signals2::scoped_connection NodeServiceImpl::SubscribeNodeFetched(
    const NodeFetchedCallback& callback) const {
  return signals_.node_fetched.connect(callback);
}

boost::signals2::scoped_connection NodeServiceImpl::SubscribeNodeStateChanged(
    const NodeStateChangedCallback& callback) const {
  return signals_.node_state_changed.connect(callback);
}

void NodeServiceImpl::NotifyModelChanged(const scada::ModelChangeEvent& event) {
  subscription_table_.EmitModelChanged(event);
  signals_.model_changed(event);
}

void NodeServiceImpl::NotifySemanticsChanged(const scada::NodeId& node_id) {
  subscription_table_.EmitNodeSemanticChanged(node_id);
  signals_.node_semantic_changed(node_id);
}

void NodeServiceImpl::NotifyNodeStateChanged(
    const NodeStateChangedEvent& event) {
  subscription_table_.EmitNodeStateChanged(event);
  signals_.node_state_changed(event);
}

void NodeServiceImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  // Service-wide observers are notified regardless of whether the node is
  // resident: consumers that don't hold a NodeRef to the node still rely on
  // remote model-change events (e.g. to decide whether to fetch it).
  NotifyModelChanged(event);

  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    // Tombstone the resident model so per-node observers see the deletion,
    // and drop our keep-alive pin: once external holders release the node it
    // unregisters itself, and a later GetNode starts from a fresh model.
    if (auto node = FindNodeModel(event.node_id))
      node->OnModelChanged(event);
    RemoveFromKeepAlive(event.node_id);
    return;
  }

  if (event.verb & scada::ModelChangeEvent::NodeAdded) {
    if (FindNodeModel(event.node_id)) {
      // TODO: Log error.
    }
    return;
  }

  if (auto node = FindNodeModel(event.node_id))
    node->OnModelChanged(event);
}

void NodeServiceImpl::OnNodeSemanticsChanged(
    const scada::SemanticChangeEvent& event) {
  // Service-wide and per-node observers both hear about every remote semantic
  // change; NotifySemanticsChanged emits the per-node signal via the
  // subscription table, so no resident model is required for routing.
  NotifySemanticsChanged(event.node_id);
}

void NodeServiceImpl::OnFetchNode(const scada::NodeId& node_id,
                                  const NodeFetchStatus& requested_status) {
  if (!channel_opened_) {
    auto& pending = pending_fetch_nodes_[node_id];
    pending.status |= requested_status;
    if (!pending.model)
      pending.model = FindNodeModel(node_id);
    return;
  }

  SpawnFetch(node_id, requested_status, FindNodeModel(node_id));
}

void NodeServiceImpl::SpawnFetch(const scada::NodeId& node_id,
                                 const NodeFetchStatus& requested_status,
                                 std::shared_ptr<NodeModelImpl> model) {
  if (requested_status.node_fetched) {
    CoSpawn(executor_,
            [this, fetcher = node_fetcher_, node_id,
             pin = model]() mutable -> Awaitable<void> {
              auto node_state = co_await fetcher->FetchNode(node_id);
              // The pin keeps the requesting model resident for the duration of
              // the fetch; if it is gone anyway, nobody is interested.
              if (!pin)
                co_return;
              if (node_state.ok()) {
                ProcessFetchedNodes({std::move(*node_state)});
              } else {
                ProcessFetchErrors({{node_id, node_state.status()}});
              }
            });
  }
  if (requested_status.children_fetched) {
    CoSpawn(executor_,
            [this, fetcher = node_fetcher_, node_id,
             pin = std::move(model)]() mutable -> Awaitable<void> {
              auto references = co_await fetcher->FetchChildren(node_id);
              if (!pin)
                co_return;
              if (references.ok()) {
                ProcessFetchedChildren(node_id, std::move(*references));
              } else {
                ProcessFetchErrors({{node_id, references.status()}});
              }
            });
  }
}

void NodeServiceImpl::ProcessFetchedNodes(
    std::vector<scada::NodeState>&& node_states) {
  // Must update all nodes at first, then notify all together.
  // TODO: Simplify

  // Fetch results apply only to resident models; hold them across both loops
  // so the OnFetched/OnFetchCompleted pairing stays balanced even if a
  // notification drops the last external pin.
  std::vector<std::shared_ptr<NodeModelImpl>> nodes;
  nodes.reserve(node_states.size());
  for (auto& node_state : node_states)
    nodes.push_back(FindNodeModel(node_state.node_id));

  for (size_t i = 0; i < node_states.size(); ++i) {
    if (nodes[i])
      nodes[i]->OnFetched(node_states[i]);
  }

  for (size_t i = 0; i < node_states.size(); ++i) {
    if (nodes[i])
      nodes[i]->OnFetchCompleted();
  }
}

void NodeServiceImpl::ProcessFetchErrors(NodeFetchStatuses&& errors) {
  for (auto& [node_id, status] : errors) {
    // Only resident models can have requested a fetch; never create a model
    // just to record an error for it.
    if (auto model = FindNodeModel(node_id))
      model->OnFetchError(std::move(status));
  }
}

void NodeServiceImpl::ProcessFetchedChildren(
    const scada::NodeId& node_id,
    scada::ReferenceDescriptions&& references) {
  if (auto node = FindNodeModel(node_id))
    node->OnChildrenFetched(std::move(references));
}

void NodeServiceImpl::OnChannelOpened() {
  base::Check(!channel_opened_);
  channel_opened_ = true;

  auto pending_fetch_nodes = std::move(pending_fetch_nodes_);
  pending_fetch_nodes_.clear();

  for (auto& [node_id, pending] : pending_fetch_nodes)
    SpawnFetch(node_id, pending.status, std::move(pending.model));
}

void NodeServiceImpl::OnChannelClosed() {
  base::Check(channel_opened_);
  base::Check(pending_fetch_nodes_.empty());

  channel_opened_ = false;
}

size_t NodeServiceImpl::GetPendingTaskCount() const {
  return pending_fetch_nodes_.size();
}

}  // namespace v3
