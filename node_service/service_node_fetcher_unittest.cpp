#include "node_service/v3/service_node_fetcher.h"

#include "address_space/test/test_address_space.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace v3 {
namespace {

using testing::Contains;
using testing::IsEmpty;
using testing::IsSupersetOf;
using testing::Not;

// Matches a ReferenceDescription by its identity fields, ignoring the target's
// browse/display names and node class (which the browse also returns).
MATCHER_P3(ReferenceIs, reference_type_id, forward, target_id, "") {
  return arg.reference_type_id == reference_type_id && arg.forward == forward &&
         arg.node_id == target_id;
}

class ServiceNodeFetcherTest : public testing::Test {
 protected:
  scada::StatusOr<scada::NodeState> FetchNode(const scada::NodeId& node_id) {
    return WaitAwaitable(executor_, fetcher_.FetchNode(node_id));
  }

  scada::StatusOr<scada::ReferenceDescriptions> FetchChildren(
      const scada::NodeId& node_id) {
    return WaitAwaitable(executor_, fetcher_.FetchChildren(node_id));
  }

  TestExecutor executor_;
  TestAddressSpace address_space_;
  ServiceNodeFetcher fetcher_{ServiceNodeFetcherContext{
      .view_service_ = address_space_.view_service_impl,
      .attribute_service_ = address_space_.attribute_service_impl,
      .service_context_ = {}}};
};

TEST_F(ServiceNodeFetcherTest, FetchNode_PopulatesCoreAttributes) {
  const auto result = FetchNode(address_space_.kTestNode2Id);

  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result->node_id, address_space_.kTestNode2Id);
  EXPECT_EQ(result->node_class, scada::NodeClass::Object);
  EXPECT_EQ(result->attributes.browse_name, "TestNode2");
  EXPECT_EQ(result->attributes.display_name, u"TestNode2DisplayName");
}

TEST_F(ServiceNodeFetcherTest, FetchNode_PopulatesTypeDefinitionAndParent) {
  const auto result = FetchNode(address_space_.kTestNode2Id);

  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result->type_definition_id, address_space_.kTestTypeId);
  EXPECT_EQ(result->parent_id, scada::id::RootFolder);
  EXPECT_EQ(result->reference_type_id, scada::id::Organizes);
  EXPECT_TRUE(result->supertype_id.is_null());
}

TEST_F(ServiceNodeFetcherTest, FetchNode_CapturesForwardNonHierarchicalReference) {
  const auto result = FetchNode(address_space_.kTestNode2Id);

  ASSERT_TRUE(result.ok());
  EXPECT_THAT(result->references,
              Contains(ReferenceIs(address_space_.kTestReferenceTypeId,
                                   /*forward=*/true,
                                   address_space_.kTestNode3Id)));
}

TEST_F(ServiceNodeFetcherTest, FetchNode_DoesNotReturnHierarchicalChildren) {
  // Children are delivered through FetchChildren, not FetchNode; the node's
  // reference list must not contain its forward hierarchical children.
  const auto result = FetchNode(address_space_.kTestNode3Id);

  ASSERT_TRUE(result.ok());
  EXPECT_THAT(result->references,
              Not(Contains(ReferenceIs(scada::id::Organizes, /*forward=*/true,
                                       address_space_.kTestNode4Id))));
  EXPECT_THAT(result->references,
              Not(Contains(ReferenceIs(scada::id::HasComponent,
                                       /*forward=*/true,
                                       address_space_.kTestNode5Id))));
}

TEST_F(ServiceNodeFetcherTest, FetchNode_TypeDefinitionHasSupertype) {
  const auto result = FetchNode(address_space_.kTestTypeId);

  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result->node_class, scada::NodeClass::ObjectType);
  EXPECT_EQ(result->supertype_id, scada::id::BaseObjectType);
}

TEST_F(ServiceNodeFetcherTest, FetchNode_ReadsPropertyDataType) {
  const auto result = FetchNode(address_space_.kTestProp1Id);

  ASSERT_TRUE(result.ok());
  EXPECT_EQ(result->node_class, scada::NodeClass::Variable);
  EXPECT_EQ(result->type_definition_id, scada::id::PropertyType);
  EXPECT_EQ(result->attributes.data_type, scada::id::String);
}

TEST_F(ServiceNodeFetcherTest, FetchNode_UnknownNode_ReturnsError) {
  const auto result = FetchNode(scada::NodeId{12345, TestAddressSpace::kNamespaceIndex});

  EXPECT_FALSE(result.ok());
}

TEST_F(ServiceNodeFetcherTest, FetchChildren_ReturnsForwardHierarchicalChildren) {
  const auto result = FetchChildren(address_space_.kTestNode3Id);

  ASSERT_TRUE(result.ok());
  // TestNode3 has an Organizes child (TestNode4) and a HasComponent child
  // (TestNode5). As a TestType instance it also gets HasProperty children, so
  // assert the hierarchical children are a superset rather than an exact set.
  EXPECT_THAT(
      *result,
      IsSupersetOf(
          {ReferenceIs(scada::id::Organizes, /*forward=*/true,
                       address_space_.kTestNode4Id),
           ReferenceIs(scada::id::HasComponent, /*forward=*/true,
                       address_space_.kTestNode5Id)}));
}

TEST_F(ServiceNodeFetcherTest, FetchChildren_LeafNodeHasNoChildren) {
  // A reference type has no hierarchical children.
  const auto result = FetchChildren(address_space_.kTestReferenceTypeId);

  ASSERT_TRUE(result.ok());
  EXPECT_THAT(*result, IsEmpty());
}

TEST_F(ServiceNodeFetcherTest, FetchChildren_UnknownNode_ReturnsNoChildren) {
  // Browsing an unknown node yields a per-browse bad status that is skipped, so
  // the fetch succeeds with an empty child set rather than failing.
  const auto result =
      FetchChildren(scada::NodeId{12345, TestAddressSpace::kNamespaceIndex});

  ASSERT_TRUE(result.ok());
  EXPECT_THAT(*result, IsEmpty());
}

}  // namespace
}  // namespace v3
