#include "common/remote_node_service.h"

#include <set>

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
    : RemoteNodeServiceContext(std::move(context)) {
  view_service_.Subscribe(*this);
}

RemoteNodeService::~RemoteNodeService() {
  view_service_.Unsubscribe(*this);
}

NodeRef RemoteNodeService::GetNode(const scada::NodeId& node_id) {
  if (node_id.is_null())
    return {};

  return GetNodeImpl(node_id, {});
}

std::shared_ptr<RemoteNodeModel> RemoteNodeService::GetNodeImpl(
    const scada::NodeId& node_id,
    const scada::NodeId& depended_id) {
  assert(!node_id.is_null());

  auto& node = nodes_[node_id];

  if (!node) {
    node = std::make_shared<RemoteNodeModel>(*this, node_id, logger_);
    node->Fetch(NodeFetchStatus::NodeOnly(), nullptr);
  }

  if (!node->fetch_status_.node_fetched && !depended_id.is_null())
    node->depended_ids_.emplace_back(depended_id);

  return node;
}

void RemoteNodeService::CompletePartialNode(
    const std::shared_ptr<RemoteNodeModel>& node) {
  if (node->fetch_status_.node_fetched)
    return;

  std::vector<scada::NodeId> fetched_node_ids;
  node->IsNodeFetched(fetched_node_ids);

  std::vector<scada::NodeId> all_dependent_ids;

  for (auto& fetched_id : fetched_node_ids) {
    auto i = nodes_.find(fetched_id);
    if (i == nodes_.end())
      continue;

    auto node = i->second;
    assert(!node->fetch_status_.node_fetched);
    assert(node->pending_request_count_ == 0);
    assert(!node->status_ || node->node_class_.has_value());
    assert(!node->status_ || !node->browse_name_.empty());

    auto callbacks = std::move(node->fetch_callbacks_);

    logger_->WriteF(LogSeverity::Normal, "Fetched node %s: %s",
                    fetched_id.ToString().c_str(),
                    node->browse_name_.name().c_str());

    std::copy(node->depended_ids_.begin(), node->depended_ids_.end(),
              std::back_inserter(all_dependent_ids));

    // Init references.
    for (auto& reference : node->pending_references_)
      node->AddReference(reference);
    node->pending_references_.clear();
    node->pending_references_.shrink_to_fit();

    node->fetch_status_.node_fetched = true;

    for (auto& o : observers_)
      o.OnNodeSemanticChanged(fetched_id);

    if (auto* node_observers = GetNodeObservers(fetched_id)) {
      for (auto& o : *node_observers)
        o.OnNodeSemanticChanged(fetched_id);
    }

    for (auto& callback : callbacks)
      callback();
  }

  for (auto& depended_id : all_dependent_ids) {
    auto i = nodes_.find(depended_id);
    if (i != nodes_.end())
      CompletePartialNode(i->second);
  }
}

void RemoteNodeService::Browse(const scada::BrowseDescription& description,
                               const BrowseCallback& callback) {
  // TODO: Cache.

  view_service_.Browse(
      {description}, [callback](const scada::Status& status,
                                std::vector<scada::BrowseResult> results) {
        if (status)
          callback(status, std::move(results.front().references));
        else
          callback(status, {});
      });
}

void RemoteNodeService::Subscribe(NodeRefObserver& observer) const {
  observers_.AddObserver(&observer);
}

void RemoteNodeService::Unsubscribe(NodeRefObserver& observer) const {
  observers_.RemoveObserver(&observer);
}

void RemoteNodeService::AddNodeObserver(const scada::NodeId& node_id,
                                        NodeRefObserver& observer) {
  node_observers_[node_id].AddObserver(&observer);
}

void RemoteNodeService::RemoveNodeObserver(const scada::NodeId& node_id,
                                           NodeRefObserver& observer) {
  node_observers_[node_id].RemoveObserver(&observer);
}

void RemoteNodeService::OnModelChanged(const scada::ModelChangeEvent& event) {
  if (auto* node_observers = GetNodeObservers(event.node_id)) {
    for (auto& o : *node_observers)
      o.OnModelChanged(event);
  }

  for (auto& o : observers_)
    o.OnModelChanged(event);
}

void RemoteNodeService::OnNodeSemanticsChanged(const scada::NodeId& node_id) {
  if (auto* node_observers = GetNodeObservers(node_id)) {
    for (auto& o : *node_observers)
      o.OnNodeSemanticChanged(node_id);
  }

  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id);
}

const RemoteNodeService::Observers* RemoteNodeService::GetNodeObservers(
    const scada::NodeId& node_id) const {
  auto i = node_observers_.find(node_id);
  return i != node_observers_.end() ? &i->second : nullptr;
}
