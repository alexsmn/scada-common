#include "common/node_state_util.h"

#include "scada/standard_node_ids.h"

namespace scada {

namespace {

struct NodeStateGraph {
  explicit NodeStateGraph(std::vector<NodeState>& nodes) : nodes{nodes} {
    for (size_t index = 0; index < nodes.size(); ++index) {
      map.try_emplace(nodes[index].node_id, index);
    }
  }

  NodeState& vertex(size_t index) { return nodes[index]; }
  size_t vertex_count() const { return nodes.size(); }

  template <typename V>
  void VisitNeighbors(size_t index, V&& visitor) const {
    const auto& node = nodes[index];

    if (!node.supertype_id.is_null()) {
      VisitHelper(id::HasSubtype, visitor);
    }

    if (!node.type_definition_id.is_null()) {
      VisitHelper(id::HasTypeDefinition, visitor);
    }

    VisitHelper(node.supertype_id, visitor);
    VisitHelper(node.reference_type_id, visitor);
    VisitHelper(node.parent_id, visitor);
    VisitHelper(node.type_definition_id, visitor);
    VisitHelper(node.attributes.data_type, visitor);
  }

  template <typename V>
  void VisitHelper(const NodeId& node_id, V&& visitor) const {
    if (node_id.is_null()) {
      return;
    }

    if (auto i = map.find(node_id); i != map.end()) {
      visitor(i->second);
    }
  }

  std::vector<NodeState>& nodes;

  std::unordered_map<NodeId, size_t /*index*/> map;
};

}  // namespace

void SortNodesHierarchically(std::vector<NodeState>& nodes) {
  struct SortVisitor {
    void Traverse() {
      sorted_indexes.reserve(graph.vertex_count());

      for (size_t index = 0; index < graph.vertex_count(); ++index) {
        VisitIndex(index);
      }
    }

    std::vector<NodeState> Collect() && {
      assert(sorted_indexes.size() == graph.vertex_count());

      std::vector<NodeState> sorted_nodes(sorted_indexes.size());
      for (size_t index = 0; index < graph.vertex_count(); ++index) {
        sorted_nodes[index] = std::move(graph.vertex(sorted_indexes[index]));
      }

      return sorted_nodes;
    }

    void VisitIndex(size_t index) {
      if (visited[index]) {
        return;
      }

      visited[index] = true;

      graph.VisitNeighbors(index,
                           std::bind_front(&SortVisitor::VisitIndex, this));

      sorted_indexes.emplace_back(index);
    }

    NodeStateGraph& graph;

    std::vector<bool> visited = std::vector<bool>(nodes.size(), false);
    std::vector<size_t /*indexes*/> sorted_indexes;
  };

  if (nodes.size() >= 2) {
    NodeStateGraph graph{nodes};
    SortVisitor visitor{graph};
    visitor.Traverse();
    nodes = std::move(visitor).Collect();
  }
}

}  // namespace scada
