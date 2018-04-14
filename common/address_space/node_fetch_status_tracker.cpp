#include "common/address_space/node_fetch_status_tracker.h"

#include "address_space/address_space.h"
#include "address_space/node_utils.h"

NodeFetchStatusTracker::NodeFetchStatusTracker(
    NodeFetchStatusTrackerContext&& context)
    : NodeFetchStatusTrackerContext{std::move(context)} {}

void NodeFetchStatusTracker::OnNodeFetched(const scada::NodeId& node_id) {
  assert(!node_id.is_null());
  assert(IsNodeFetched(node_id));

  // Process as parent.
  const auto status = GetStatus(node_id);
  assert(status.node_fetched);
  node_fetch_status_changed_handler_(node_id, status);

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
    auto status = GetStatus(parent_id);
    assert(status.children_fetched);
    node_fetch_status_changed_handler_(parent_id, status);
  }
}

void NodeFetchStatusTracker::Delete(const scada::NodeId& node_id) {
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

NodeFetchStatus NodeFetchStatusTracker::GetStatus(
    const scada::NodeId& node_id) const {
  NodeFetchStatus status{};
  status.node_fetched = IsNodeFetched(node_id);
  auto i = parents_.find(node_id);
  status.children_fetched = i != parents_.end() && i->second.empty();
  return status;
}

bool NodeFetchStatusTracker::IsNodeFetched(const scada::NodeId& node_id) const {
  return address_space_.GetNode(node_id) != nullptr;
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
    node_fetch_status_changed_handler_(parent_id, GetStatus(parent_id));
}
