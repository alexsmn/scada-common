#include "address_space/node_utils.h"

#include "address_space/object.h"
#include "address_space/test/test_address_space.h"
#include "model/node_id_util.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

namespace scada {

TEST(NodeUtils, GetForwardReferenceNodes) {
  TestAddressSpace address_space;

  auto* parent = address_space.GetNode(address_space.kTestNode2Id);
  auto* property1 = address_space.GetNode(MakeNestedNodeId(
      address_space.kTestNode2Id, address_space.kTestProp1BrowseName));
  auto* property2 = address_space.GetNode(MakeNestedNodeId(
      address_space.kTestNode2Id, address_space.kTestProp2BrowseName));

  ASSERT_TRUE(parent);
  ASSERT_TRUE(property1);
  ASSERT_TRUE(property2);
  ASSERT_TRUE(
      scada::FindReference(*parent, address_space.kTestReferenceTypeId, true));

  EXPECT_THAT(GetForwardReferenceNodes(*parent, scada::id::Aggregates),
              UnorderedElementsAre(property1, property2));
}

}  // namespace scada
