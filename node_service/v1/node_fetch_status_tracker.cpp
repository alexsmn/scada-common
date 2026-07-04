#include "node_service/v1/node_fetch_status_tracker.h"

#include "address_space/address_space.h"
#include "address_space/node_utils.h"
#include "base/check.h"
#include "base/debug_util.h"
#include "base/range_util.h"


namespace v1 {

// NodeFetchStatusTracker

NodeFetchStatusTracker::NodeFetchStatusTracker(
    NodeFetchStatusTrackerContext&& context)
    : NodeFetchStatusTrackerContext{std::move(context)} {}

void NodeFetchStatusTracker::SetFetchStatusesHint(
    const NodeFetchStatuses& errors,
    const std::vector<std::pair<scada::NodeId, NodeFetchStatus>>&
        fetch_statuses) {
  for (const auto& [node_id, new_fetch_status] : fetch_statuses) {
    base::Check(!node_id.is_null(), "fetch status hint for null node");
    base::Check(!new_fetch_status.empty(), "empty fetch status hint");
    experimental_fetch_statuses_.insert_or_assign(
        node_id, std::make_pair(scada::StatusCode::Good, new_fetch_status));
  }

  for (const auto& [node_id, new_error_status] : errors) {
    base::Check(!node_id.is_null(), "error status hint for null node");
    base::Check(!new_error_status, "good status passed as error hint");
    experimental_fetch_statuses_.insert_or_assign(
        node_id, std::make_pair(new_error_status, NodeFetchStatus::Max()));
  }
}

void NodeFetchStatusTracker::OnNodesFetched(const NodeFetchStatuses& statuses) {
  LOG_INFO(logger_) << "Nodes fetched"
                    << LOG_TAG("statuses", ToString(statuses));

  base::Check(!statuses.empty(), "OnNodesFetched with no statuses");
  thread_checker_.CheckCalledOnValidThread();

  auto status_lock = status_queue_.Lock();

  for (auto& [node_id, status] : statuses) {
    base::Check(!node_id.is_null(), "fetched status for null node");
    base::Check(!status || IsNodeFetched(node_id),
                "good status for a node absent from the address space");

    if (status)
      errors_.erase(node_id);
    else
      errors_.insert_or_assign(node_id, std::move(status));

    // Process as parent.
    status_queue_.NotifyStatusChanged(node_id);

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

  thread_checker_.CheckCalledOnValidThread();
  base::Check(!parent_id.is_null(), "children fetched for null parent");

  auto status_lock = status_queue_.Lock();

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
    status_queue_.NotifyStatusChanged(parent_id);
}

void NodeFetchStatusTracker::Delete(const scada::NodeId& node_id) {
  LOG_INFO(logger_) << "Delete" << LOG_TAG("node_id", ToString(node_id));

  thread_checker_.CheckCalledOnValidThread();

  auto status_lock = status_queue_.Lock();

  errors_.erase(node_id);
  experimental_fetch_statuses_.erase(node_id);
  status_queue_.CancelPendingStatus(node_id);

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
  auto result = GetStatusHelper(node_id);

  auto experimental_result = GetExperimentalStatus(node_id);
  base::Check(result.first == experimental_result.first,
              "experimental status diverged");
  base::Check(
      result.second.node_fetched == experimental_result.second.node_fetched,
      "experimental node_fetched diverged");
  base::Check(result.second.non_hierarchical_inverse_references ==
                  experimental_result.second.non_hierarchical_inverse_references,
              "experimental inverse references diverged");

  return result;
}

std::pair<scada::Status, NodeFetchStatus>
NodeFetchStatusTracker::GetStatusHelper(const scada::NodeId& node_id) const {
  thread_checker_.CheckCalledOnValidThread();

  if (auto i = errors_.find(node_id); i != errors_.end()) {
    base::Check(!i->second, "good status stored in error map");
    return {i->second, NodeFetchStatus::Max()};
  }

  NodeFetchStatus fetch_status{};
  fetch_status.node_fetched = IsNodeFetched(node_id);

  if (auto i = parents_.find(node_id); i != parents_.end())
    fetch_status.children_fetched = i->second.empty();

  return {scada::StatusCode::Good, std::move(fetch_status)};
}

std::pair<scada::Status, NodeFetchStatus>
NodeFetchStatusTracker::GetExperimentalStatus(
    const scada::NodeId& node_id) const {
  auto i = experimental_fetch_statuses_.find(node_id);
  return i != experimental_fetch_statuses_.end()
             ? i->second
             : std::make_pair(scada::Status{scada::StatusCode::Good},
                              NodeFetchStatus{});
}

bool NodeFetchStatusTracker::IsNodeFetched(const scada::NodeId& node_id) const {
  return address_space_.GetNode(node_id) != nullptr ||
         errors_.find(node_id) != errors_.end();
}

void NodeFetchStatusTracker::OnChildFetched(const scada::NodeId& child_id) {
  base::Check(!child_id.is_null(), "child fetched for null node");

  auto i = children_.find(child_id);
  if (i == children_.end())
    return;

  const auto parent_ids = std::move(i->second);
  children_.erase(i);

  for (const auto& parent_id : parent_ids) {
    base::Check(parents_.find(parent_id) != parents_.end(),
                "child links to unknown parent");
    auto& pending_child_ids = parents_[parent_id];
    base::Check(pending_child_ids.find(child_id) != pending_child_ids.end(),
                "parent does not list child as pending");
    pending_child_ids.erase(child_id);

    if (pending_child_ids.empty())
      status_queue_.NotifyStatusChanged(parent_id);
  }
}

}  // namespace v1
