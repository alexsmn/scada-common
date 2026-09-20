#include "address_space/address_space_util.h"

#include "scada/standard_node_ids.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <set>
#include <vector>

namespace scada {
namespace {

// Regression: the breadth-first walk seeded its queue with the root but not
// its `seen` set, so any neighbour referencing back to the root enqueued it a
// second time and the root came out twice.
//
// Nothing noticed for a long time because the two callers that would have
// cared hide it: the nodeset golden dump keys a std::map by node id, and
// AddAll on a node service is idempotent. The one that does not hide it is
// SaveAddressSpaceXml's non-AddressSpaceImpl branch, which writes exactly
// what this returns — a duplicated RootFolder element in the saved file.
TEST(AddressSpaceUtilTest, EveryNodeIsVisitedExactlyOnce) {
  // The standard address space, which is what AddressSpaceImpl2 carries on
  // construction and the smallest graph with a root that is referenced back.
  const std::vector<NodeState> nodes = MakeStandardNodeStates();
  ASSERT_FALSE(nodes.empty());

  std::set<NodeId> unique;
  for (const NodeState& node : nodes)
    unique.insert(node.node_id);

  EXPECT_EQ(nodes.size(), unique.size());
}

TEST(AddressSpaceUtilTest, TheRootItselfIsVisitedExactlyOnce) {
  // Named separately because the root is the one the seeding bug duplicated,
  // so a count-only assertion could pass again while it regressed.
  const std::vector<NodeState> nodes = MakeStandardNodeStates();
  const auto roots = std::ranges::count_if(
      nodes, [](const NodeState& node) {
        return node.node_id == NodeId{id::RootFolder};
      });

  EXPECT_EQ(1, roots);
}

}  // namespace
}  // namespace scada
