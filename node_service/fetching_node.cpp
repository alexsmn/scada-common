#include "node_service/fetching_node.h"

namespace {

template <class T>
inline void Erase(std::vector<T>& c, const T& v) {
  auto i = std::find(c.begin(), c.end(), v);
  if (i != c.end())
    c.erase(i);
}

}  // namespace

void FetchingNode::ClearDependentNodes() {
  for (auto* dependent_node : dependent_nodes)
    Erase(dependent_node->depends_of, this);
  dependent_nodes.clear();
}

void FetchingNode::ClearDependsOf() {
  for (auto* depends_of : depends_of)
    Erase(depends_of->dependent_nodes, this);
  depends_of.clear();
}
