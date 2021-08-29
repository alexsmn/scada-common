#pragma once

#include "node_service/fetching_node.h"
#include "node_service/node_fetcher.h"

class FetchingNodeGraph {
 public:
  std::size_t size() const { return fetching_nodes_.size(); }

  FetchingNode* FindNode(const scada::NodeId& node_id);
  FetchingNode& AddNode(const scada::NodeId& node_id);

  void RemoveNode(const scada::NodeId& node_id);

  void AddDependency(FetchingNode& node, FetchingNode& from);

  std::pair<std::vector<scada::NodeState> /*fetched_nodes*/,
            NodeFetchStatuses /*errors*/>
  GetFetchedNodes();

  bool AssertValid() const;

  std::string GetDebugString() const;

 private:
  std::map<scada::NodeId, FetchingNode> fetching_nodes_;

  int fetch_cache_iteration_ = 1;
};
