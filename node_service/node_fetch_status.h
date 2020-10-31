#pragma once

#include <ostream>

struct NodeFetchStatus {
  NodeFetchStatus() {}
  NodeFetchStatus(bool node_fetched, bool children_fetched)
      : node_fetched{node_fetched}, children_fetched{children_fetched} {}

  static NodeFetchStatus Max() { return NodeFetchStatus{true, true}; }
  static NodeFetchStatus NodeOnly() { return NodeFetchStatus{true, false}; }
  static NodeFetchStatus NodeAndChildren() {
    return NodeFetchStatus{true, true};
  }

  bool empty() const { return !node_fetched && !children_fetched; }

  bool any_less(const NodeFetchStatus& other) const {
    return children_fetched < other.children_fetched ||
           node_fetched < other.node_fetched;
  }

  bool all_less_or_equal(const NodeFetchStatus& other) const {
    return children_fetched <= other.children_fetched &&
           node_fetched <= other.node_fetched;
  }

  bool node_fetched = false;
  bool children_fetched = false;
};

inline bool operator==(const NodeFetchStatus& a, const NodeFetchStatus& b) {
  return a.children_fetched == b.children_fetched &&
         a.node_fetched == b.node_fetched;
}

inline bool operator!=(const NodeFetchStatus& a, const NodeFetchStatus& b) {
  return !(a == b);
}

inline NodeFetchStatus& operator|=(NodeFetchStatus& a,
                                   const NodeFetchStatus& b) {
  a.node_fetched |= b.node_fetched;
  a.children_fetched |= b.children_fetched;
  return a;
}

inline std::ostream& operator<<(std::ostream& stream,
                                const NodeFetchStatus& status) {
  return stream << status.node_fetched << '/' << status.children_fetched;
}
