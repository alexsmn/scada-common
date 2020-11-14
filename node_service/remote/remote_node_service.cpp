#include "node_service/remote/remote_node_service.h"

#include "core/attribute_service.h"
#include "core/standard_node_ids.h"
#include "node_service/node_observer.h"
#include "node_service/node_util.h"
#include "node_service/remote/remote_node_model.h"

RemoteNodeService::RemoteNodeService(RemoteNodeServiceContext&& context)
    : RemoteNodeServiceContext(std::move(context)),
      node_fetcher_{NodeFetcherImpl::Create(MakeNodeFetcherImplContext())},
      node_children_fetcher_{
          NodeChildrenFetcher::Create(MakeNodeChildrenFetcherContext())} {}

RemoteNodeService::~RemoteNodeService() {}

NodeRef RemoteNodeService::GetNode(const scada::NodeId& node_id) {
  auto node = GetNodeModel(node_id);
  if (!node)
    return nullptr;

  node->Fetch(NodeFetchStatus::NodeOnly(), {});
  return node;
}

std::shared_ptr<RemoteNodeModel> RemoteNodeService::GetNodeModel(
    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  auto& node = nodes_[node_id];
  if (!node)
    node = std::make_shared<RemoteNodeModel>(*this, node_id);

  return node;
}

void RemoteNodeService::Subscribe(NodeRefObserver& observer) const {
  assert(!observers_.HasObserver(&observer));
  observers_.AddObserver(&observer);
}

void RemoteNodeService::Unsubscribe(NodeRefObserver& observer) const {
  observers_.RemoveObserver(&observer);
}

void RemoteNodeService::NotifyModelChanged(
    const scada::ModelChangeEvent& event) {
  for (auto& o : observers_)
    o.OnModelChanged(event);
}

void RemoteNodeService::NotifySemanticsChanged(const scada::NodeId& node_id) {
  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id);
}

void RemoteNodeService::OnModelChanged(const scada::ModelChangeEvent& event) {
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

void RemoteNodeService::OnNodeSemanticsChanged(
    const scada::SemanticChangeEvent& event) {
  if (auto i = nodes_.find(event.node_id); i != nodes_.end())
    i->second->OnNodeSemanticChanged();
}

void RemoteNodeService::OnFetchNode(const scada::NodeId& node_id,
                                    const NodeFetchStatus& requested_status) {
  if (!channel_opened_) {
    pending_fetch_nodes_[node_id] |= requested_status;
    return;
  }

  if (requested_status.node_fetched)
    node_fetcher_->Fetch(node_id);
  if (requested_status.children_fetched)
    node_children_fetcher_->Fetch(node_id);
}

void RemoteNodeService::ProcessFetchedNodes(
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

void RemoteNodeService::ProcessFetchErrors(NodeFetchStatuses&& errors) {
  for (auto& [node_id, status] : errors) {
    auto& model = nodes_[node_id];
    model->OnFetchError(std::move(status));
  }
}

NodeFetcherImplContext RemoteNodeService::MakeNodeFetcherImplContext() {
  auto fetch_completed_handler =
      [this](std::vector<scada::NodeState>&& node_states,
             NodeFetchStatuses&& errors) {
        ProcessFetchedNodes(std::move(node_states));
        ProcessFetchErrors(std::move(errors));
      };

  NodeValidator node_validator = [this](const scada::NodeId& node_id) -> bool {
    return nodes_.find(node_id) != nodes_.end();
  };

  return {
      io_context_,
      executor_,
      view_service_,
      attribute_service_,
      fetch_completed_handler,
      node_validator,
  };
}

NodeChildrenFetcherContext RemoteNodeService::MakeNodeChildrenFetcherContext() {
  ReferenceValidator reference_validator =
      [this](const scada::NodeId& node_id, scada::BrowseResult&& result) {
        if (auto node = GetNodeModel(node_id))
          node->OnChildrenFetched(std::move(result.references));
      };

  return {io_context_, executor_, view_service_, reference_validator};
}

void RemoteNodeService::OnChannelOpened() {
  assert(!channel_opened_);
  channel_opened_ = true;

  auto pending_fetch_nodes = std::move(pending_fetch_nodes_);
  pending_fetch_nodes_.clear();

  for (auto& [node_id, requested_status] : pending_fetch_nodes) {
    if (requested_status.node_fetched)
      node_fetcher_->Fetch(node_id);
    if (requested_status.children_fetched)
      node_children_fetcher_->Fetch(node_id);
  }
}

void RemoteNodeService::OnChannelClosed() {
  assert(channel_opened_);
  assert(pending_fetch_nodes_.empty());

  channel_opened_ = false;
}
