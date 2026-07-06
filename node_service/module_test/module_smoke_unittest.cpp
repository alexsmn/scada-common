// Smoke test for the scada.node_service module facade: names come from
// `import scada.node_service;` only, plus the documented textual include
// for the global NodeFetchStatus operators.

// Global-namespace operator sets (here: NodeFetchStatus comparisons) are
// never exported; include the header textually alongside the import.
#include "node_service/node_fetch_status.h"

#include <unordered_map>

#include <gtest/gtest.h>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.node_service;

namespace scada_node_service_module {
namespace {

TEST(ScadaNodeServiceModuleSmoke, NodeRefAndFetchStatus) {
  NodeRef node;
  EXPECT_TRUE(node.fetched());  // out-of-line, links across the boundary

  // std::hash<NodeRef> reachability (kept alive by the facade).
  std::unordered_map<NodeRef, int> map;
  EXPECT_TRUE(map.empty());

  NodeFetchStatus status = NodeFetchStatus::NodeOnly();
  EXPECT_TRUE(status == NodeFetchStatus::NodeOnly());  // textual operator==
}

TEST(ScadaNodeServiceModuleSmoke, TransitiveSurfaces) {
  scada::NodeProperties properties;  // scada.common via export import
  EXPECT_TRUE(properties.empty());
  EXPECT_EQ(Format(4), "4");
  base::Check(true, "node_service module smoke");
}

}  // namespace
}  // namespace scada_node_service_module
