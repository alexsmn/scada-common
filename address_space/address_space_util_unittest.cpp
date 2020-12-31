#include "address_space_util.h"

#include "common/node_state.h"
#include "core/node_class.h"

#include <gmock/gmock.h>

TEST(AddressSpaceUtil, SortNodesHierarchically) {
  std::vector<scada::NodeState> nodes = {
      {1, scada::NodeClass::Object, 0, 2},
      {2, scada::NodeClass::Object, 0, 3},
      {3, scada::NodeClass::Object, 0, 4},
  };

  SortNodesHierarchically(nodes);

  ASSERT_EQ(static_cast<size_t>(3), nodes.size());
  EXPECT_EQ(static_cast<scada::NumericId>(3), nodes[0].node_id.numeric_id());
  EXPECT_EQ(static_cast<scada::NumericId>(2), nodes[1].node_id.numeric_id());
  EXPECT_EQ(static_cast<scada::NumericId>(1), nodes[2].node_id.numeric_id());
}
