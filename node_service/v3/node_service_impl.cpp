#include "node_service/v3/node_service_impl.h"

#include "core/attribute_service.h"
#include "core/standard_node_ids.h"
#include "node_service/node_observer.h"
#include "node_service/node_util.h"
#include "node_service/v3/node_model_impl.h"
#include "node_service/v3/node_fetcher.h"

namespace v3 {

NodeServiceImpl::NodeServiceImpl(NodeServiceImplContext&& context)
    : NodeServiceImplContext(std::move(context)) {}

NodeServiceImpl::~NodeServiceImpl() {}

NodeRef NodeServiceImpl::GetNode(const scada::NodeId& node_id) {
  auto node = GetNodeModel(node_id);
  if (!node)
    return nullptr;

  node->Fetch(NodeFetchStatus::NodeOnly(), {});
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

  if (requested_status.node_fetched)
    node_fetcher_->FetchNode(node_id);
  if (requested_status.children_fetched)
    node_fetcher_->FetchChildren(node_id);
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

void NodeServiceImpl::OnChannelOpened() {
  assert(!channel_opened_);
  channel_opened_ = true;

  auto pending_fetch_nodes = std::move(pending_fetch_nodes_);
  pending_fetch_nodes_.clear();

  for (auto& [node_id, requested_status] : pending_fetch_nodes) {
    if (requested_status.node_fetched)
      node_fetcher_->FetchNode(node_id);
    if (requested_status.children_fetched)
      node_fetcher_->FetchChildren(node_id);
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
