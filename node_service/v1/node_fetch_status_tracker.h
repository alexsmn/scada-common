#pragma once

#include "base/boost_log.h"
#include "base/threading/thread_checker.h"
#include "core/configuration_types.h"
#include "core/view_service.h"
#include "node_service/node_fetch_status.h"
#include "node_service/v1/node_fetch_status_queue.h"
#include "node_service/v1/node_fetch_status_types.h"

#include <functional>
#include <map>
#include <set>

namespace scada {
class AddressSpace;
class Node;
}  // namespace scada

namespace v1 {

using NodeFetchStatuses = std::vector<std::pair<scada::NodeId, scada::Status>>;

using NodeValidator = std::function<bool(const scada::NodeId& node_id)>;

struct NodeFetchStatusTrackerContext {
  const NodeFetchStatusChangedHandler node_fetch_status_changed_handler_;
  const NodeValidator node_validator_;
  scada::AddressSpace& address_space_;
};

class NodeFetchStatusTracker : private NodeFetchStatusTrackerContext {
 public:
  explicit NodeFetchStatusTracker(NodeFetchStatusTrackerContext&& context);

  // For good statuses the node must exist in the address space.
  void OnNodesFetched(const NodeFetchStatuses& statuses);

  void OnChildrenFetched(const scada::NodeId& node_id,
                         scada::ReferenceDescriptions&& references);

  void Delete(const scada::NodeId& node_id);

  std::pair<scada::Status, NodeFetchStatus> GetStatus(
      const scada::NodeId& node_id) const;

 private:
  bool IsNodeFetched(const scada::NodeId& node_id) const;

  void DeleteNodeStatesRecursive(const scada::Node& node);

  void OnChildFetched(const scada::NodeId& child_id);

  BoostLogger logger_{LOG_NAME("NodeFetchStatusTracker")};

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

  // Status notification consolidation queue.
  NodeFetchStatusQueue status_queue_{
      node_fetch_status_changed_handler_,
      [this](const scada::NodeId& node_id) { return GetStatus(node_id); }};
};

}  // namespace v1
