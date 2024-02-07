#include "address_space_util.h"

#include "common/node_state.h"
#include "scada/node_class.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

template <std::ranges::range R>
inline auto ToVector(R&& r) {
  return std::vector(std::begin(r), std::end(r));
}

TEST(AddressSpaceUtil, SortNodesHierarchically_Parents) {
  std::vector<scada::NodeState> nodes = {
      {.node_id = 1, .parent_id = 2},
      {.node_id = 2, .parent_id = 3},
      {.node_id = 3, .parent_id = 4},
  };

  SortNodesHierarchically(nodes);

  auto sorted_node_ids =
      ToVector(nodes | std::views::transform(&scada::NodeState::node_id));

  EXPECT_THAT(sorted_node_ids, ElementsAre(3, 2, 1));
}

TEST(AddressSpaceUtil, SortNodesHierarchically_TypeDefinitions) {
  std::vector<scada::NodeState> nodes = {
      {.node_id = 1, .type_definition_id = 2},
      {.node_id = 2, .type_definition_id = 3},
      {.node_id = 3, .type_definition_id = 4},
      {.node_id = 4},
      {.node_id = scada::id::HasTypeDefinition}};

  SortNodesHierarchically(nodes);

  auto sorted_node_ids =
      ToVector(nodes | std::views::transform(&scada::NodeState::node_id));

  EXPECT_THAT(sorted_node_ids,
              ElementsAre(scada::id::HasTypeDefinition, 4, 3, 2, 1));
}
