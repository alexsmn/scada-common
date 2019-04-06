#include "common/remote_node_service.h"

#include "base/logger.h"
#include "common/node_id_util.h"
#include "core/attribute_service.h"
#include "core/test/test_address_space.h"

#include <gmock/gmock.h>

#include "core/debug_util-inl.h"

using namespace testing;

/*struct NodeRefServiceImplTestContext {
  const std::shared_ptr<Logger> logger = std::make_shared<NullLogger>();
  TestAddressSpace address_space;
  TestViewService view_service{address_space};
  MockAttributeService attribute_service;
  RemoteNodeService node_service{NodeRefServiceImplContext{
      logger,
      view_service,
      attribute_service,
  }};
};*/

TEST(RemoteNodeService, FetchTypeDefinition) {
  const auto logger = std::make_shared<NullLogger>();
  TestAddressSpace address_space;
  RemoteNodeService node_service{RemoteNodeServiceContext{
      logger,
      address_space,
      address_space,
  }};

  node_service.OnChannelOpened();

  auto node = node_service.GetNode(address_space.kTestTypeId);
  node.Fetch(NodeFetchStatus::NodeOnly());
  ASSERT_TRUE(node);
  EXPECT_TRUE(node.fetched());
  EXPECT_EQ(node.node_class(), scada::NodeClass::ObjectType);

  auto prop1 = node[address_space.kTestProp1Id];
  ASSERT_TRUE(prop1);
  EXPECT_TRUE(prop1.fetched());
  EXPECT_EQ(prop1.browse_name().name(), address_space.kTestProp1BrowseName);
}

TEST(RemoteNodeService, FetchObject) {
  const auto logger = std::make_shared<NullLogger>();
  TestAddressSpace address_space;
  RemoteNodeService node_service{RemoteNodeServiceContext{
      logger,
      address_space,
      address_space,
  }};

  node_service.OnChannelOpened();

  auto node = node_service.GetNode(address_space.kTestNode1Id);
  node.Fetch(NodeFetchStatus::NodeOnly());
  ASSERT_TRUE(node);
  EXPECT_TRUE(node.fetched());
  EXPECT_EQ(node.node_class(), scada::NodeClass::Object);

  auto type_definition = node.type_definition();
  ASSERT_TRUE(type_definition);
  EXPECT_TRUE(type_definition.fetched());
  EXPECT_EQ(address_space.kTestTypeId, type_definition.node_id());

  auto prop1 = node[address_space.kTestProp1Id];
  ASSERT_TRUE(prop1);
  EXPECT_TRUE(prop1.fetched());
  EXPECT_EQ(prop1.browse_name().name(), address_space.kTestProp1BrowseName);
  EXPECT_EQ(prop1.value(), "TestNode1.TestProp1.Value");
}
