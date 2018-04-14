#include "common/address_space/address_space_fetcher.h"

#include <gmock/gmock.h>

#include "base/logger.h"
#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/standard_address_space.h"
#include "core/test/test_address_space.h"
#include "address_space/variable.h"

using namespace testing;

struct TestContext {
  TestContext() {
    // Create custom types, since we don't transfer types.
    client_address_space.AddStaticNode(std::make_unique<scada::ObjectType>(
        server_address_space.kTestTypeId, "TestType", L"TestType"));
    auto* string_node =
        scada::AsDataType(client_address_space.GetNode(scada::id::String));
    assert(string_node);
    client_address_space.AddStaticNode(std::make_unique<scada::GenericVariable>(
        server_address_space.kTestProp1Id, "TestProp1", L"TestProp1DisplayName",
        *string_node));
    scada::AddReference(client_address_space, scada::id::HasProperty,
                        server_address_space.kTestTypeId,
                        server_address_space.kTestProp1Id);
    client_address_space.AddStaticNode(std::make_unique<scada::GenericVariable>(
        server_address_space.kTestProp2Id, "TestProp2", L"TestProp2DisplayName",
        *string_node));
    scada::AddReference(client_address_space, scada::id::HasProperty,
                        server_address_space.kTestTypeId,
                        server_address_space.kTestProp2Id);
    client_address_space.AddStaticNode(std::make_unique<scada::ReferenceType>(
        server_address_space.kTestRefTypeId, "TestRef", L"TestRef"));

    for (auto& node : server_address_space.nodes) {
      EXPECT_CALL(*this, OnNodeFetchStatusChanged(node.node_id,
                                                  NodeFetchStatus::NodeOnly()));
      EXPECT_CALL(*this, OnNodeFetchStatusChanged(
                             node.node_id, NodeFetchStatus::NodeAndChildren()));
    }

    fetcher.OnChannelOpened();
    fetcher.FetchNode(scada::id::RootFolder,
                      NodeFetchStatus::NodeAndChildren());
    Mock::VerifyAndClearExpectations(this);
  }

  MOCK_METHOD2(OnNodeFetchStatusChanged,
               void(const scada::NodeId& node_id,
                    const NodeFetchStatus& status));

  const std::shared_ptr<Logger> logger = std::make_shared<NullLogger>();

  test::TestAddressSpace server_address_space;

  AddressSpaceImpl client_address_space{logger};

  GenericNodeFactory node_factory{logger, client_address_space};

  AddressSpaceFetcher fetcher{AddressSpaceFetcherContext{
      logger,
      server_address_space,
      server_address_space,
      client_address_space,
      node_factory,
      false,
      [this](const scada::NodeId& node_id, const NodeFetchStatus& status) {
        OnNodeFetchStatusChanged(node_id, status);
      },
  }};
};

TEST(AddressSpaceFetcher, ConfigurationLoad) {
  TestContext context;

  {
    auto* node = context.client_address_space.GetNode(scada::id::RootFolder);
    ASSERT_NE(nullptr, node);
    EXPECT_NE(nullptr, FindChild(*node, "TestNode1"));
    EXPECT_NE(nullptr, FindChild(*node, "TestNode2"));
  }

  {
    auto* node = context.client_address_space.GetNode(
        context.server_address_space.kTestNode1Id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->GetBrowseName(), "TestNode1");
    EXPECT_EQ(
        scada::Variant{"TestNode1.TestProp1.Value"},
        node->GetPropertyValue(context.server_address_space.kTestProp1Id));
    EXPECT_TRUE(
        node->GetPropertyValue(context.server_address_space.kTestProp2Id)
            .is_null());
  }

  {
    auto* node = context.client_address_space.GetNode(
        context.server_address_space.kTestNode2Id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->GetBrowseName(), "TestNode2");
    EXPECT_EQ(
        scada::Variant{"TestNode2.TestProp1.Value"},
        node->GetPropertyValue(context.server_address_space.kTestProp1Id));
    EXPECT_EQ(
        scada::Variant{"TestNode2.TestProp2.Value"},
        node->GetPropertyValue(context.server_address_space.kTestProp2Id));
  }
}

TEST(AddressSpaceFetcher, NodeAdded) {
  TestContext context;

  const scada::NodeState kNewNode{
      scada::NodeId{"NewNodeId", 0},
      scada::NodeClass::Object,
      context.server_address_space.kTestTypeId,
      scada::id::RootFolder,
      scada::id::Organizes,
      scada::NodeAttributes{}.set_browse_name("NewNode"),
  };

  EXPECT_CALL(context, OnNodeFetchStatusChanged(kNewNode.node_id,
                                                NodeFetchStatus::NodeOnly()));
  EXPECT_CALL(context,
              OnNodeFetchStatusChanged(kNewNode.node_id,
                                       NodeFetchStatus::NodeAndChildren()));

  context.server_address_space.nodes.emplace_back(kNewNode);
  context.server_address_space.NotifyModelChanged(
      {kNewNode.node_id, kNewNode.type_definition_id,
       scada::ModelChangeEvent::NodeAdded});

  context.fetcher.FetchNode(kNewNode.node_id,
                            NodeFetchStatus::NodeAndChildren());

  {
    auto* node = context.client_address_space.GetNode(kNewNode.node_id);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->GetBrowseName(), "NewNode");
  }
}

TEST(AddressSpaceFetcher, NodeDeleted) {
  TestContext context;

  const scada::NodeId kNodeId = context.server_address_space.kTestNode1Id;
  ASSERT_NE(nullptr, context.server_address_space.GetNode(kNodeId));

  context.server_address_space.NotifyModelChanged(
      {kNodeId, {}, scada::ModelChangeEvent::NodeDeleted});

  EXPECT_EQ(nullptr, context.client_address_space.GetNode(kNodeId));
}

TEST(AddressSpaceFetcher, NodeSemanticsChanged) {
  TestContext context;

  const scada::NodeId kNodeId = context.server_address_space.kTestNode1Id;
  const scada::QualifiedName kNewBrowseName{"NewTestNode1"};
  const scada::Variant kNewValue{"TestNode1.TestProp1.NewValue"};

  {
    auto* node = context.server_address_space.GetNode(kNodeId);
    // Rename
    EXPECT_EQ("TestNode1", node->attributes.browse_name);
    node->attributes.browse_name = kNewBrowseName;
    // Change property
    auto& prop = node->properties[0];
    EXPECT_EQ(context.server_address_space.kTestProp1Id, prop.first);
    EXPECT_EQ(scada::Variant{"TestNode1.TestProp1.Value"}, prop.second);
    prop.second = kNewValue;
  }

  context.server_address_space.NotifyNodeSemanticsChanged(kNodeId);

  {
    auto* node = context.client_address_space.GetNode(kNodeId);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(kNewBrowseName, node->GetBrowseName());
    EXPECT_EQ(kNewValue, node->GetPropertyValue(
                             context.server_address_space.kTestProp1Id));
  }
}

TEST(AddressSpaceFetcher, DeleteNodeByDeletionOfParentReference) {
  TestContext context;

  const scada::NodeId kParentNodeId = context.server_address_space.kTestNode3Id;
  const scada::NodeId kNodeId = context.server_address_space.kTestNode4Id;

  // Delete node, but notify only deleted reference deletion.
  // Expect node is deleted on client.

  auto* node = context.server_address_space.GetNode(kNodeId);
  ASSERT_NE(node, nullptr);
  const auto saved_node = *node;
  context.server_address_space.DeleteNode(kNodeId);
  context.server_address_space.NotifyModelChanged(
      {kParentNodeId, {}, scada::ModelChangeEvent::ReferenceDeleted});

  EXPECT_EQ(nullptr, context.client_address_space.GetNode(kNodeId));

  // Restore reference. Expect node is refetched on client.

  EXPECT_CALL(context,
              OnNodeFetchStatusChanged(kNodeId, NodeFetchStatus::NodeOnly()));
  EXPECT_CALL(context, OnNodeFetchStatusChanged(
                           kNodeId, NodeFetchStatus::NodeAndChildren()));

  context.server_address_space.nodes.emplace_back(saved_node);
  context.server_address_space.NotifyModelChanged(
      {kParentNodeId, {}, scada::ModelChangeEvent::ReferenceAdded});

  EXPECT_NE(nullptr, context.client_address_space.GetNode(kNodeId));
}
