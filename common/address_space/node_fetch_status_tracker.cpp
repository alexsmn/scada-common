#include "common/address_space/node_fetch_status_tracker.h"

#include "address_space/address_space.h"
#include "address_space/node_utils.h"

NodeFetchStatusTracker::NodeFetchStatusTracker(
    NodeFetchStatusTrackerContext&& context)
    : NodeFetchStatusTrackerContext{std::move(context)} {}

void NodeFetchStatusTracker::OnNodeFetched(const scada::NodeId& node_id,
                                           scada::Status status) {
  assert(!node_id.is_null());
  assert(!status || IsNodeFetched(node_id));

  if (status)
    errors_.erase(node_id);
  else
    errors_.insert_or_assign(node_id, std::move(status));

  // Process as parent.
  const auto [new_status, fetch_status] = GetStatus(node_id);
  assert(fetch_status.node_fetched);
  node_fetch_status_changed_handler_(node_id, std::move(new_status),
                                     std::move(fetch_status));

  // Process as child.
  OnChildFetched(node_id);
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

  if (pending_child_ids.empty()) {
    auto [status, fetch_status] = GetStatus(parent_id);
    assert(fetch_status.children_fetched);
    node_fetch_status_changed_handler_(parent_id, status, fetch_status);
  }
}

void NodeFetchStatusTracker::Delete(const scada::NodeId& node_id) {
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

  if (pending_child_ids.empty()) {
    auto [status, fetch_status] = GetStatus(parent_id);
    node_fetch_status_changed_handler_(parent_id, std::move(status),
                                       std::move(fetch_status));
  }
}
