#pragma once

#include <ostream>

// Which parts of a node have been (or should be) fetched — a set of independent
// fetch dimensions combined as bit flags. Union with operator|; test whether
// one status already covers another with Includes().
enum class NodeFetchStatus : unsigned {
  None = 0,
  NodeOnly = 1u << 0,                          // the node's own attributes
  ChildrenOnly = 1u << 1,                      // hierarchical child references
  NonHierarchicalInverseReferences = 1u << 2,  // inverse non-hierarchical refs

  NodeAndChildren = NodeOnly | ChildrenOnly,
  Max = NodeOnly | ChildrenOnly | NonHierarchicalInverseReferences,
};

constexpr NodeFetchStatus operator|(NodeFetchStatus a, NodeFetchStatus b) {
  return static_cast<NodeFetchStatus>(static_cast<unsigned>(a) |
                                      static_cast<unsigned>(b));
}

constexpr NodeFetchStatus operator&(NodeFetchStatus a, NodeFetchStatus b) {
  return static_cast<NodeFetchStatus>(static_cast<unsigned>(a) &
                                      static_cast<unsigned>(b));
}

constexpr NodeFetchStatus& operator|=(NodeFetchStatus& a, NodeFetchStatus b) {
  return a = a | b;
}

// True when |status| requests/reports nothing.
constexpr bool IsEmpty(NodeFetchStatus status) {
  return status == NodeFetchStatus::None;
}

// True when |have| already covers every dimension in |want| (i.e. a fetch that
// reached |have| satisfies a request for |want|).
constexpr bool Includes(NodeFetchStatus have, NodeFetchStatus want) {
  return (have & want) == want;
}

std::ostream& operator<<(std::ostream& stream, NodeFetchStatus status);
