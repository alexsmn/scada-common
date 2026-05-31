#include "node_service/v3/node_service_impl.h"

#include "base/awaitable.h"
#include "scada/attribute_service.h"
#include "scada/standard_node_ids.h"
#include "node_service/node_observer.h"
#include "node_service/node_util.h"
#include "node_service/v3/node_model_impl.h"
#include "node_service/v3/node_fetcher.h"

namespace v3 {

namespace {

std::unique_ptr<IViewEventsSubscription> MakeViewEventsSubscription(
    scada::MonitoredItemService& monitored_item_service,
    scada::ViewEvents& events,
    const ViewEventsProvider& provider) {
  if (provider)
    return provider(events);
  return std::make_unique<ViewEventsSubscription>(monitored_item_service,
                                                  events);
}

}  // namespace

NodeServiceImpl::NodeServiceImpl(NodeServiceImplContext&& context)
    : NodeServiceImplContext(std::move(context)),
      view_events_subscription_{MakeViewEventsSubscription(
          monitored_item_service_, *this, view_events_provider_)} {}

NodeServiceImpl::~NodeServiceImpl() {}

NodeRef NodeServiceImpl::GetNode(const scada::NodeId& node_id) {
  auto node = GetNodeModel(node_id);
  if (!node)
    return nullptr;

  node->StartFetch(NodeFetchStatus::NodeOnly());
  return node;
}

std::shared_ptr<NodeModelImpl> NodeServiceImpl::GetNodeModel(
    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  auto& node = nodes_[node_id];
  if (!node)
    node = std::make_shared<NodeModelImpl>(*this, node_id);

  return node;
}

void NodeServiceImpl::Subscribe(NodeRefObserver& observer) const {
  assert(!observers_.HasObserver(&observer));
  observers_.AddObserver(&observer);
}

void NodeServiceImpl::Unsubscribe(NodeRefObserver& observer) const {
  observers_.RemoveObserver(&observer);
}

void NodeServiceImpl::NotifyModelChanged(const scada::ModelChangeEvent& event) {
  for (auto& o : observers_)
    o.OnModelChanged(event);
}

void NodeServiceImpl::NotifySemanticsChanged(const scada::NodeId& node_id) {
  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id);
}

void NodeServiceImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (event.verb & scada::ModelChangeEvent::NodeDeleted) {
    if (auto i = nodes_.find(event.node_id); i != nodes_.end()) {
      /*auto model = std::move(i->second);
      nodes_.erase(i);
      model->OnModelChanged(
          {event.node_id, {}, scada::ModelChangeEvent::NodeDeleted});*/
    }
    return;
  }

  if (event.verb & scada::ModelChangeEvent::NodeAdded) {
    if (nodes_.find(event.node_id) != nodes_.end()) {
      // TODO: Log error.
      return;
    }
    return;
  }

  if (auto i = nodes_.find(event.node_id); i != nodes_.end())
    i->second->OnModelChanged(event);
}

void NodeServiceImpl::OnNodeSemanticsChanged(
    const scada::SemanticChangeEvent& event) {
  if (auto i = nodes_.find(event.node_id); i != nodes_.end())
    i->second->OnNodeSemanticChanged();
}

void NodeServiceImpl::OnFetchNode(const scada::NodeId& node_id,
                                  const NodeFetchStatus& requested_status) {
  if (!channel_opened_) {
    pending_fetch_nodes_[node_id] |= requested_status;
    return;
  }

  if (requested_status.node_fetched) {
    CoSpawn(executor_, [this, fetcher = node_fetcher_, node_id]() mutable
                         -> Awaitable<void> {
      auto node_state = co_await fetcher->FetchNode(node_id);
      if (node_state.ok()) {
        ProcessFetchedNodes({std::move(*node_state)});
      } else {
        ProcessFetchErrors({{node_id, node_state.status()}});
      }
    });
  }
  if (requested_status.children_fetched) {
    CoSpawn(executor_, [this, fetcher = node_fetcher_, node_id]() mutable
                         -> Awaitable<void> {
      auto references = co_await fetcher->FetchChildren(node_id);
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

  for (auto& node_state : node_states) {
    if (auto node = GetNodeModel(node_state.node_id))
      node->OnFetched(node_state);
  }

  for (auto& node_state : node_states) {
    if (auto node = GetNodeModel(node_state.node_id))
      node->OnFetchCompleted();
  }
}

void NodeServiceImpl::ProcessFetchErrors(NodeFetchStatuses&& errors) {
  for (auto& [node_id, status] : errors) {
    auto& model = nodes_[node_id];
    model->OnFetchError(std::move(status));
  }
}

void NodeServiceImpl::ProcessFetchedChildren(
    const scada::NodeId& node_id,
    scada::ReferenceDescriptions&& references) {
  if (auto node = GetNodeModel(node_id))
    node->OnChildrenFetched(std::move(references));
}

void NodeServiceImpl::OnChannelOpened() {
  assert(!channel_opened_);
  channel_opened_ = true;

  auto pending_fetch_nodes = std::move(pending_fetch_nodes_);
  pending_fetch_nodes_.clear();

  for (auto& [node_id, requested_status] : pending_fetch_nodes) {
    if (requested_status.node_fetched) {
      CoSpawn(executor_, [this, fetcher = node_fetcher_, node_id]() mutable
                           -> Awaitable<void> {
        auto node_state = co_await fetcher->FetchNode(node_id);
        if (node_state.ok()) {
          ProcessFetchedNodes({std::move(*node_state)});
        } else {
          ProcessFetchErrors({{node_id, node_state.status()}});
        }
      });
    }
    if (requested_status.children_fetched) {
      CoSpawn(executor_, [this, fetcher = node_fetcher_, node_id]() mutable
                           -> Awaitable<void> {
        auto references = co_await fetcher->FetchChildren(node_id);
        if (references.ok()) {
          ProcessFetchedChildren(node_id, std::move(*references));
        } else {
          ProcessFetchErrors({{node_id, references.status()}});
        }
      });
    }
  }
}

void NodeServiceImpl::OnChannelClosed() {
  assert(channel_opened_);
  assert(pending_fetch_nodes_.empty());

  channel_opened_ = false;
}

size_t NodeServiceImpl::GetPendingTaskCount() const {
  return pending_fetch_nodes_.size();
}

}  // namespace v3
