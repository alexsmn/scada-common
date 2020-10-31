#include "node_service/address_space/node_fetch_status_tracker.h"

#include "address_space/address_space.h"
#include "address_space/node_utils.h"

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
  ++tracker_.status_lock_count_;
}

NodeFetchStatusTracker::ScopedStatusLock::~ScopedStatusLock() {
  assert(tracker_.status_lock_count_ > 0);
  --tracker_.status_lock_count_;
  if (tracker_.status_lock_count_ == 0)
    tracker_.FetchPendingStatuses();
}

// NodeFetchStatusTracker

NodeFetchStatusTracker::NodeFetchStatusTracker(
    NodeFetchStatusTrackerContext&& context)
    : NodeFetchStatusTrackerContext{std::move(context)} {}

void NodeFetchStatusTracker::OnNodesFetched(const NodeFetchStatuses& statuses) {
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
    std::set<scada::NodeId> child_ids) {
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
  for (auto& child_id : child_ids) {
    if (!IsNodeFetched(child_id)) {
      pending_child_ids.insert(child_id);
      assert(children_.find(child_id) == children_.end());
      children_.emplace(child_id, parent_id);
    }
  }

  if (pending_child_ids.empty())
    NotifyStatusChanged(parent_id);
}

void NodeFetchStatusTracker::Delete(const scada::NodeId& node_id) {
  ScopedStatusLock lock{*this};

  errors_.erase(node_id);

  // Process as parent.
  if (auto i = parents_.find(node_id); i != parents_.end()) {
    auto& pending_child_ids = i->second;
    for (auto& child_id : pending_child_ids)
      children_.erase(child_id);
    parents_.erase(i);
  }

  // Process as child.
  OnChildFetched(node_id);

  if (auto* node = address_space_.GetNode(node_id))
    DeleteNodeStatesRecursive(*node);
}

std::pair<scada::Status, NodeFetchStatus> NodeFetchStatusTracker::GetStatus(
    const scada::NodeId& node_id) const {
  scada::Status status{scada::StatusCode::Good};

  NodeFetchStatus fetch_status{};
  fetch_status.node_fetched = IsNodeFetched(node_id);

  if (auto i = errors_.find(node_id); i != errors_.end()) {
    assert(!i->second);
    status = i->second;
  }

  if (auto i = parents_.find(node_id); i != parents_.end())
    fetch_status.children_fetched = i->second.empty();

  return {std::move(status), std::move(fetch_status)};
}

bool NodeFetchStatusTracker::IsNodeFetched(const scada::NodeId& node_id) const {
  return address_space_.GetNode(node_id) != nullptr ||
         errors_.find(node_id) != errors_.end();
}

void NodeFetchStatusTracker::OnChildFetched(const scada::NodeId& child_id) {
  assert(!child_id.is_null());

  auto i = children_.find(child_id);
  if (i == children_.end())
    return;

  const auto parent_id = std::move(i->second);
  children_.erase(i);

  assert(parents_.find(parent_id) != parents_.end());
  auto& pending_child_ids = parents_[parent_id];
  assert(pending_child_ids.find(child_id) != pending_child_ids.end());
  pending_child_ids.erase(child_id);

  if (pending_child_ids.empty())
    NotifyStatusChanged(parent_id);
}

void NodeFetchStatusTracker::NotifyStatusChanged(const scada::NodeId& node_id) {
  if (status_lock_count_ == 0) {
    auto [status, fetch_status] = GetStatus(node_id);
    node_fetch_status_changed_handler_(node_id, status, fetch_status);
    return;
  }

  pending_statuses_.emplace(node_id);
}

void NodeFetchStatusTracker::FetchPendingStatuses() {
  assert(status_lock_count_ >= 0);

  if (status_lock_count_ != 0)
    return;

  auto statuses = std::move(pending_statuses_);
  pending_statuses_.clear();

  for (auto& node_id : statuses) {
    auto [status, fetch_status] = GetStatus(node_id);
    node_fetch_status_changed_handler_(node_id, status, fetch_status);
  }
}
