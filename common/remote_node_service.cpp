#include "common/remote_node_service.h"

#include "base/bind.h"
#include "base/logger.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "common/node_observer.h"
#include "common/node_util.h"
#include "common/remote_node_model.h"
#include "core/attribute_service.h"
#include "core/standard_node_ids.h"

RemoteNodeService::RemoteNodeService(RemoteNodeServiceContext&& context)
    : RemoteNodeServiceContext(std::move(context)),
      node_fetcher_{MakeNodeFetcherImplContext()},
      node_children_fetcher_{MakeNodeChildrenFetcherContext()} {}

RemoteNodeService::~RemoteNodeService() {}

NodeRef RemoteNodeService::GetNode(const scada::NodeId& node_id) {
  if (node_id.is_null())
    return {};

  return GetNodeImpl(node_id);
}

std::shared_ptr<RemoteNodeModel> RemoteNodeService::GetNodeImpl(
    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return nullptr;

  auto& node = nodes_[node_id];

  if (!node) {
    node = std::make_shared<RemoteNodeModel>(*this, node_id);
    //    node->Fetch(NodeFetchStatus::Max(), nullptr);
  }

  return node;
}

void RemoteNodeService::Subscribe(NodeRefObserver& observer) const {
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
      auto model = std::move(i->second);
      nodes_.erase(i);
      model->OnModelChanged(
          {event.node_id, {}, scada::ModelChangeEvent::NodeDeleted});
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

void RemoteNodeService::OnNodeSemanticsChanged(const scada::NodeId& node_id) {
  if (auto i = nodes_.find(node_id); i != nodes_.end())
    i->second->OnNodeSemanticChanged();
}

void RemoteNodeService::OnFetchNode(const scada::NodeId& node_id,
                                    const NodeFetchStatus& requested_status) {
  if (!channel_opened_) {
    pending_fetch_nodes_[node_id] |= requested_status;
    return;
  }

  if (requested_status.node_fetched)
    node_fetcher_.Fetch(node_id);
  if (requested_status.children_fetched)
    node_children_fetcher_.Fetch(node_id);
}

void RemoteNodeService::ProcessFetchedNodes(
    std::vector<scada::NodeState>&& node_states) {
  // Must update all nodes at first, then notify all together.
  // TODO: Simplify

  for (auto& node_state : node_states) {
    auto& model = nodes_[node_state.node_id];
    if (!model)
      model = std::make_shared<RemoteNodeModel>(*this, node_state.node_id);
    model->OnNodeFetched(std::move(node_state));
  }

  for (auto& node_state : node_states) {
    auto i = nodes_.find(node_state.node_id);
    if (i != nodes_.end()) {
      auto fetch_status = i->second->GetFetchStatus();
      if (!fetch_status.node_fetched) {
        fetch_status.node_fetched = true;
        i->second->SetFetchStatus(scada::StatusCode::Good, fetch_status);
      }
    }
  }
}

void RemoteNodeService::ProcessFetchErrors(NodeFetchStatuses&& errors) {
  for (auto& [node_id, status] : errors) {
    auto& model = nodes_[node_id];
    if (!model)
      model = std::make_shared<RemoteNodeModel>(*this, node_id);
    model->SetFetchStatus(std::move(status), NodeFetchStatus::Max());
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

  return {logger_, view_service_, attribute_service_, fetch_completed_handler,
          node_validator};
}

NodeChildrenFetcherContext RemoteNodeService::MakeNodeChildrenFetcherContext() {
  ReferenceValidator reference_validator = [this](const scada::NodeId& node_id,
                                                  ReferenceMap references) {
    if (auto i = nodes_.find(node_id); i != nodes_.end())
      i->second->OnChildrenFetched(std::move(references));
  };

  return {logger_, view_service_, reference_validator};
}

void RemoteNodeService::OnChannelOpened() {
  assert(!channel_opened_);
  channel_opened_ = true;

  auto pending_fetch_nodes = std::move(pending_fetch_nodes_);
  pending_fetch_nodes_.clear();

  for (auto& [node_id, requested_status] : pending_fetch_nodes) {
    if (requested_status.node_fetched)
      node_fetcher_.Fetch(node_id);
    if (requested_status.children_fetched)
      node_children_fetcher_.Fetch(node_id);
  }
}

void RemoteNodeService::OnChannelClosed() {
  assert(channel_opened_);
  assert(pending_fetch_nodes_.empty());

  channel_opened_ = false;
}
