#include "node_service/fetching_node_graph.h"

namespace {

template <class T>
inline void Insert(std::vector<T>& c, const T& v) {
  if (std::find(c.begin(), c.end(), v) == c.end())
    c.push_back(v);
}

}  // namespace

FetchingNode* FetchingNodeGraph::FindNode(const scada::NodeId& node_id) {
  assert(!node_id.is_null());

  auto i = fetching_nodes_.find(node_id);
  return i != fetching_nodes_.end() ? &i->second : nullptr;
}

FetchingNode& FetchingNodeGraph::AddNode(const scada::NodeId& node_id) {
  assert(!node_id.is_null());

  auto p = fetching_nodes_.try_emplace(node_id);
  auto& node = p.first->second;
  if (p.second)
    node.node_id = node_id;

  return node;
}

void FetchingNodeGraph::RemoveNode(const scada::NodeId& node_id) {
  assert(AssertValid());

  auto i = fetching_nodes_.find(node_id);
  if (i == fetching_nodes_.end())
    return;

  auto& node = i->second;
  node.ClearDependsOf();
  node.ClearDependentNodes();

  fetching_nodes_.erase(i);

  assert(AssertValid());
}

std::pair<std::vector<scada::NodeState> /*fetched_nodes*/,
          NodeFetchStatuses /*errors*/>
FetchingNodeGraph::GetFetchedNodes() {
  assert(AssertValid());

  struct Collector {
    bool IsFetchedRecursively(FetchingNode& node) {
      if (!node.fetched())
        return false;

      if (node.fetch_cache_iteration == fetch_cache_iteration)
        return node.fetch_cache_state;

      // A cycle means fetched as well.
      node.fetch_cache_iteration = fetch_cache_iteration;
      node.fetch_cache_state = true;

      bool fetched = true;
      for (auto* depends_of : node.depends_of) {
        if (!IsFetchedRecursively(*depends_of)) {
          fetched = false;
          break;
        }
      }

      node.fetch_cache_state = fetched;

      return fetched;
    }

    FetchingNodeGraph& graph;
    int fetch_cache_iteration = 0;
  };

  std::vector<scada::NodeState> fetched_nodes;
  fetched_nodes.reserve(std::min<size_t>(32, fetching_nodes_.size()));

  NodeFetchStatuses errors;

  Collector collector{*this};
  for (auto i = fetching_nodes_.begin(); i != fetching_nodes_.end();) {
    auto& node = i->second;

    collector.fetch_cache_iteration = fetch_cache_iteration_++;
    if (!collector.IsFetchedRecursively(node)) {
      ++i;
      continue;
    }

    assert(node.fetch_started);

    if (node.status)
      fetched_nodes.emplace_back(node);
    else
      errors.emplace_back(node.node_id, node.status);

    node.ClearDependsOf();
    node.ClearDependentNodes();
    i = fetching_nodes_.erase(i);
  }

  assert(AssertValid());

  return {std::move(fetched_nodes), std::move(errors)};
}

void FetchingNodeGraph::AddDependency(FetchingNode& node, FetchingNode& from) {
  assert(&node != &from);

  Insert(from.dependent_nodes, &node);
  Insert(node.depends_of, &from);
}

bool FetchingNodeGraph::AssertValid() const {
  return true;
}

std::string FetchingNodeGraph::GetDebugString() const {
  std::stringstream stream;
  for (auto& p : fetching_nodes_) {
    auto& node = p.second;
    stream << node.node_id << ": ";
    for (auto* depends_of : node.depends_of)
      stream << depends_of->node_id << ", ";
    stream << std::endl;
  }
  return stream.str();
}
