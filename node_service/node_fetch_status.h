#pragma once

#include <ostream>

struct NodeFetchStatus {
  constexpr static NodeFetchStatus Max() {
    return NodeFetchStatus{true, true, true};
  }
  constexpr static NodeFetchStatus NodeOnly() {
    return NodeFetchStatus{true, false};
  }
  constexpr static NodeFetchStatus NodeAndChildren() {
    return NodeFetchStatus{true, true};
  }
  constexpr static NodeFetchStatus ChildrenOnly() {
    return NodeFetchStatus{false, true};
  }

  constexpr bool empty() const { return !node_fetched && !children_fetched; }

  constexpr bool any_less(NodeFetchStatus other) const {
    return children_fetched < other.children_fetched ||
           node_fetched < other.node_fetched ||
           non_hierarchical_inverse_references <
               other.non_hierarchical_inverse_references;
  }

  constexpr bool all_less_or_equal(NodeFetchStatus other) const {
    return children_fetched <= other.children_fetched &&
           node_fetched <= other.node_fetched &&
           non_hierarchical_inverse_references <=
               other.non_hierarchical_inverse_references;
  }

  bool node_fetched = false;
  bool children_fetched = false;
  bool non_hierarchical_inverse_references = false;
};

inline constexpr bool operator==(NodeFetchStatus a, NodeFetchStatus b) {
  return a.children_fetched == b.children_fetched &&
         a.node_fetched == b.node_fetched &&
         a.non_hierarchical_inverse_references ==
             b.non_hierarchical_inverse_references;
}

inline constexpr bool operator!=(NodeFetchStatus a, NodeFetchStatus b) {
  return !(a == b);
}

inline constexpr NodeFetchStatus& operator|=(NodeFetchStatus& a,
                                             NodeFetchStatus b) {
  a.node_fetched |= b.node_fetched;
  a.children_fetched |= b.children_fetched;
  a.non_hierarchical_inverse_references |=
      b.non_hierarchical_inverse_references;
  return a;
}

inline std::ostream& operator<<(std::ostream& stream, NodeFetchStatus status) {
  return stream << status.node_fetched << '/' << status.children_fetched << '/'
                << status.non_hierarchical_inverse_references;
}
