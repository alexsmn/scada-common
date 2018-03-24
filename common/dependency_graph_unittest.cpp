#include <gmock/gmock.h>

#include <queue>

using namespace testing;

class DependencyGraph {
 public:
  using Node = int;

  void AddDependency(const Node& node, const Node& parent);
  void SetComplete(const Node& node, std::vector<Node>& complete_nodes);

 private:
  struct NodeInfo;

  using Nodes = std::map<Node, NodeInfo>;

  struct NodeInfo {
    bool recursively_complete() const { return complete && parents.empty(); }

    bool complete = false;
    bool visited = false;
    std::vector<Nodes::iterator> parents;
    std::vector<Nodes::iterator> children;
  };

  Nodes nodes_;
};

void DependencyGraph::AddDependency(const Node& node, const Node& parent) {
  auto ni = nodes_.try_emplace(node).first;
  auto pi = nodes_.try_emplace(parent).first;

  ni->second.parents.emplace_back(pi);
  pi->second.children.emplace_back(ni);
}

void DependencyGraph::SetComplete(const Node& node,
                                  std::vector<Node>& complete_nodes) {
  auto ni = nodes_.try_emplace(node).first;

  auto& complete = ni->second.complete;
  if (complete)
    return;
  complete = true;

  if (!ni->second.recursively_complete())
    return;

  std::vector<Nodes::iterator> nodes;
  nodes.emplace_back(ni);
  for (std::size_t p = 0; p < nodes.size(); ++p) {
    auto& node = nodes[p]->second;
    if (node.visited)
      continue;

    for (auto& ci : node.children) {
      auto& child = ci->second;
      if (!child.visited && child.recursively_complete()) {
        child.visited = true;
        nodes.emplace_back(ci);
      }
    }
  }

  complete_nodes.reserve(complete_nodes.size() + nodes.size());
  for (auto& ni : nodes) {
    ni->second.visited = false;
    complete_nodes.emplace_back(ni->first);
  }
}

TEST(DependencyGraph, Test) {
  DependencyGraph graph;
  graph.AddDependency(1, 2);
  graph.AddDependency(2, 3);
  std::vector<DependencyGraph::Node> complete_nodes;
  graph.SetComplete(1, complete_nodes);
  EXPECT_THAT(complete_nodes, ElementsAre());
  graph.SetComplete(2, complete_nodes);
  EXPECT_THAT(complete_nodes, ElementsAre());
  graph.SetComplete(3, complete_nodes);
  EXPECT_THAT(complete_nodes, UnorderedElementsAre(1, 2, 3));
}
