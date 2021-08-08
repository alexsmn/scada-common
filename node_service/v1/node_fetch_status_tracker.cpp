#include "node_service/v1/node_fetch_status_tracker.h"

#include "address_space/address_space.h"
#include "address_space/node_utils.h"
#include "base/auto_reset.h"
#include "base/debug_util.h"
#include "base/range_util.h"

#include "base/debug_util-inl.h"

namespace v1 {

// NodeFetchStatusTracker::ScopedStatusLock

class NodeFetchStatusTracker::ScopedStatusLock {
 public:
  explicit ScopedStatusLock(NodeFetchStatusTracker& tracker);
  ~ScopedStatusLock();

  ScopedStatusLock(const ScopedStatusLock&) = delete;
  ScopedStatusLock& operator=(const ScopedStatusLock&) = delete;

 private:
  NodeFetchStatusTracker& tracker_;
};

NodeFetchStatusTracker::ScopedStatusLock::ScopedStatusLock(
    NodeFetchStatusTracker& tracker)
    : tracker_{tracker} {
  assert(tracker_.thread_checker_.CalledOnValidThread());
  ++tracker_.status_lock_count_;
}

NodeFetchStatusTracker::ScopedStatusLock::~ScopedStatusLock() {
  assert(tracker_.thread_checker_.CalledOnValidThread());
  assert(tracker_.status_lock_count_ > 0);
  --tracker_.status_lock_count_;
  if (tracker_.status_lock_count_ == 0)
    tracker_.NotifyPendingStatusChanged();
}

// NodeFetchStatusTracker

NodeFetchStatusTracker::NodeFetchStatusTracker(
    NodeFetchStatusTrackerContext&& context)
    : NodeFetchStatusTrackerContext{std::move(context)} {}

void NodeFetchStatusTracker::OnNodesFetched(const NodeFetchStatuses& statuses) {
  LOG_INFO(logger_) << "Nodes fetched"
                    << LOG_TAG("statuses", ToString(statuses));

  assert(!statuses.empty());
  assert(thread_checker_.CalledOnValidThread());

  ScopedStatusLock lock{*this};

  for (auto& [node_id, status] : statuses) {
    assert(!node_id.is_null());
    assert(!status || IsNodeFetched(node_id));

    if (status)
      errors_.erase(node_id);
    else
      errors_.insert_or_assign(node_id, std::move(status));

    // Process as parent.
    NotifyStatusChanged(node_id);

    // Process as child.
    OnChildFetched(node_id);
  }
}

void NodeFetchStatusTracker::DeleteNodeStatesRecursive(
    const scada::Node& node) {
  for (auto* child : scada::GetChildren(node)) {
    Delete(child->id());
    DeleteNodeStatesRecursive(*child);
  }
}

void NodeFetchStatusTracker::OnChildrenFetched(
    const scada::NodeId& parent_id,
    scada::ReferenceDescriptions&& references) {
  LOG_INFO(logger_) << "Children fetched"
                    << LOG_TAG("parent_id", ToString(parent_id))
                    << LOG_TAG("references.size", references.size());
  LOG_DEBUG(logger_) << "Children fetched"
                     << LOG_TAG("parent_id", ToString(parent_id))
                     << LOG_TAG("references.size", references.size())
                     << LOG_TAG("references", ToString(references));

  assert(thread_checker_.CalledOnValidThread());
  assert(!notifying_);
  assert(!parent_id.is_null());

  ScopedStatusLock lock{*this};

  auto [i, inserted] = parents_.try_emplace(parent_id);
  auto& pending_child_ids = i->second;
  // Cancel if the node's children are already fetched.
  if (!inserted && pending_child_ids.empty())
    return;

  // Remove old pending children.
  for (auto& child_id : pending_child_ids)
    children_.erase(child_id);
  pending_child_ids.clear();

  // Insert new pending children.
  for (auto& reference : references) {
    if (reference.reference_type_id != parent_id &&
        !IsNodeFetched(reference.reference_type_id)) {
      pending_child_ids.emplace(reference.reference_type_id);
      children_[reference.reference_type_id].emplace(parent_id);
      node_validator_(reference.reference_type_id);
    }
    if (reference.node_id != parent_id && !IsNodeFetched(reference.node_id)) {
      pending_child_ids.emplace(reference.node_id);
      children_[reference.node_id].emplace(parent_id);
      node_validator_(reference.node_id);
    }
  }

  if (pending_child_ids.empty())
    NotifyStatusChanged(parent_id);
}

void NodeFetchStatusTracker::Delete(const scada::NodeId& node_id) {
  LOG_INFO(logger_) << "Delete" << LOG_TAG("node_id", ToString(node_id));

  assert(thread_checker_.CalledOnValidThread());
  assert(!notifying_);

  ScopedStatusLock lock{*this};

  errors_.erase(node_id);

  // Process as parent.
  if (auto i = parents_.find(node_id); i != parents_.end()) {
    auto& pending_child_ids = i->second;
    for (auto& child_id : pending_child_ids) {
      auto j = children_.find(child_id);
      auto& parent_ids = j->second;
      parent_ids.erase(node_id);
      if (parent_ids.empty())
        children_.erase(j);
    }
    parents_.erase(i);
  }

  // Process as child.
  OnChildFetched(node_id);

  if (auto* node = address_space_.GetNode(node_id))
    DeleteNodeStatesRecursive(*node);
}

std::pair<scada::Status, NodeFetchStatus> NodeFetchStatusTracker::GetStatus(
    const scada::NodeId& node_id) const {
  assert(thread_checker_.CalledOnValidThread());

  scada::Status status{scada::StatusCode::Good};

  NodeFetchStatus fetch_status{};
  fetch_status.node_fetched = IsNodeFetched(node_id);

  if (auto i = errors_.find(node_id); i != errors_.end()) {
    assert(!i->second);
    status = i->second;
  }

  if (status) {
    if (auto i = parents_.find(node_id); i != parents_.end())
      fetch_status.children_fetched = i->second.empty();
  } else {
    fetch_status.children_fetched = true;
  }

  return {std::move(status), std::move(fetch_status)};
}

bool NodeFetchStatusTracker::IsNodeFetched(const scada::NodeId& node_id) const {
  return address_space_.GetNode(node_id) != nullptr ||
         errors_.find(node_id) != errors_.end();
}

void NodeFetchStatusTracker::OnChildFetched(const scada::NodeId& child_id) {
  assert(!notifying_);
  assert(!child_id.is_null());

  auto i = children_.find(child_id);
  if (i == children_.end())
    return;

  const auto parent_ids = std::move(i->second);
  children_.erase(i);

  for (const auto& parent_id : parent_ids) {
    assert(parents_.find(parent_id) != parents_.end());
    auto& pending_child_ids = parents_[parent_id];
    assert(pending_child_ids.find(child_id) != pending_child_ids.end());
    pending_child_ids.erase(child_id);

    if (pending_child_ids.empty())
      NotifyStatusChanged(parent_id);
  }
}

void NodeFetchStatusTracker::NotifyStatusChanged(const scada::NodeId& node_id) {
  LOG_INFO(logger_) << "Node status changed"
                    << LOG_TAG("NodeId", ToString(node_id));

  if (status_lock_count_ == 0) {
    auto [status, fetch_status] = GetStatus(node_id);
    NodeFetchStatusChangedItem item{node_id, status, fetch_status};
    node_fetch_status_changed_handler_(base::span{&item, 1});
    return;
  }

  pending_statuses_.emplace(node_id);
}

void NodeFetchStatusTracker::NotifyPendingStatusChanged() {
  assert(!notifying_);
  assert(status_lock_count_ >= 0);

  if (status_lock_count_ != 0)
    return;

  if (pending_statuses_.empty())
    return;

  base::AutoReset notifying{&notifying_, true};

  auto statuses = std::move(pending_statuses_);
  pending_statuses_.clear();

  /*auto items = Map(statuses, [this](const scada::NodeId& node_id) {
    auto [status, fetch_status] = GetStatus(node_id);
    return NodeFetchStatusChangedItem{node_id, std::move(status), fetch_status};
  });*/

  std::vector<NodeFetchStatusChangedItem> items;
  items.reserve(statuses.size());
  for (const scada::NodeId& node_id : statuses) {
    auto [status, fetch_status] = GetStatus(node_id);
    items.emplace_back(
        NodeFetchStatusChangedItem{node_id, std::move(status), fetch_status});
  }

  node_fetch_status_changed_handler_(items);
}

}  // namespace v1
