#pragma once

#include "base/containers/span.h"
#include "base/threading/thread_checker.h"
#include "core/configuration_types.h"
#include "core/node_id.h"
#include "core/status.h"
#include "core/view_service.h"
#include "node_service/node_fetch_status.h"

#include <functional>
#include <map>
#include <set>

namespace scada {
class AddressSpace;
class Node;
}  // namespace scada

using NodeFetchStatuses = std::vector<std::pair<scada::NodeId, scada::Status>>;

struct NodeFetchStatusChangedItem {
  scada::NodeId node_id;
  scada::Status status;
  NodeFetchStatus fetch_status;
};

using NodeFetchStatusChangedHandler =
    std::function<void(base::span<const NodeFetchStatusChangedItem> items)>;

using NodeValidator = std::function<bool(const scada::NodeId& node_id)>;

struct NodeFetchStatusTrackerContext {
  const NodeFetchStatusChangedHandler node_fetch_status_changed_handler_;
  const NodeValidator node_validator_;
  scada::AddressSpace& address_space_;
};

class NodeFetchStatusTracker : private NodeFetchStatusTrackerContext {
 public:
  explicit NodeFetchStatusTracker(NodeFetchStatusTrackerContext&& context);

  void OnNodesFetched(const NodeFetchStatuses& statuses);
  void OnChildrenFetched(const scada::NodeId& node_id,
                         scada::ReferenceDescriptions&& references);

  void Delete(const scada::NodeId& node_id);

  std::pair<scada::Status, NodeFetchStatus> GetStatus(
      const scada::NodeId& node_id) const;

 private:
  class ScopedStatusLock;

  bool IsNodeFetched(const scada::NodeId& node_id) const;

  void DeleteNodeStatesRecursive(const scada::Node& node);

  void OnChildFetched(const scada::NodeId& child_id);

  void NotifyStatusChanged(const scada::NodeId& node_id);

  void NotifyPendingStatusChanged();

  base::ThreadChecker thread_checker_;

  // Key is absent - not fetched, unknown node
  // Key presents, pending_child_ids.empty() - fetched
  // Key presents, !pending_child_ids.empty() - pending children
  std::map<scada::NodeId /*parent_id*/,
           std::set<scada::NodeId> /*pending_child_ids*/>
      parents_;

  std::map<scada::NodeId /*child_id*/, std::set<scada::NodeId> /*parent_ids*/>
      children_;

  std::map<scada::NodeId, scada::Status> errors_;

  std::set<scada::NodeId> pending_statuses_;
  bool notifying_ = false;
  int status_lock_count_ = 0;
};
