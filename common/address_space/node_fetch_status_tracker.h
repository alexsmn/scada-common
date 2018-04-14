#pragma once

#include "common/node_fetch_status.h"
#include "core/node_id.h"

#include <functional>
#include <map>
#include <set>

namespace scada {
class AddressSpace;
class Node;
}

using NodeFetchStatusChangedHandler =
    std::function<void(const scada::NodeId& node_id,
                       const NodeFetchStatus& status)>;

struct NodeFetchStatusTrackerContext {
  const NodeFetchStatusChangedHandler node_fetch_status_changed_handler_;
  scada::AddressSpace& address_space_;
};

class NodeFetchStatusTracker : private NodeFetchStatusTrackerContext {
 public:
  explicit NodeFetchStatusTracker(NodeFetchStatusTrackerContext&& context);

  void OnNodeFetched(const scada::NodeId& node_id);
  void OnChildrenFetched(const scada::NodeId& node_id,
                         std::set<scada::NodeId> child_ids);

  void Delete(const scada::NodeId& node_id);

  NodeFetchStatus GetStatus(const scada::NodeId& node_id) const;

 private:
  bool IsNodeFetched(const scada::NodeId& node_id) const;

  void DeleteNodeStatesRecursive(const scada::Node& node);

  void OnChildFetched(const scada::NodeId& child_id);

  // Key is absent - not fetched, unknown node
  // Key presents, pending_child_ids.empty() - fetched
  // Key presents, !pending_child_ids.empty() - pending children
  std::map<scada::NodeId /*parent_id*/,
           std::set<scada::NodeId> /*pending_child_ids*/>
      parents_;

  std::map<scada::NodeId /*child_id*/, scada::NodeId /*parent_id*/> children_;
};
