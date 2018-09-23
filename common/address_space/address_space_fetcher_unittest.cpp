#include "common/address_space/address_space_fetcher.h"

#include <gmock/gmock.h>

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/standard_address_space.h"
#include "address_space/variable.h"
#include "base/logger.h"
#include "base/strings/utf_string_conversions.h"
#include "core/test/test_address_space.h"

using namespace testing;

namespace {

struct TestContext {
  TestContext() {
    // Create custom types, since we don't transfer types.
    client_address_space.AddStaticNode(std::make_unique<scada::ObjectType>(
        server_address_space.kTestTypeId, "TestType",
        base::WideToUTF16(L"TestType")));
    auto* string_node =
        scada::AsDataType(client_address_space.GetNode(scada::id::String));
    assert(string_node);
    client_address_space.AddStaticNode(std::make_unique<scada::GenericVariable>(
        server_address_space.kTestProp1Id, "TestProp1",
        base::WideToUTF16(L"TestProp1DisplayName"), *string_node));
    scada::AddReference(client_address_space, scada::id::HasProperty,
                        server_address_space.kTestTypeId,
                        server_address_space.kTestProp1Id);
    client_address_space.AddStaticNode(std::make_unique<scada::GenericVariable>(
        server_address_space.kTestProp2Id, "TestProp2",
        base::WideToUTF16(L"TestProp2DisplayName"), *string_node));
    scada::AddReference(client_address_space, scada::id::HasProperty,
                        server_address_space.kTestTypeId,
                        server_address_space.kTestProp2Id);
    client_address_space.AddStaticNode(std::make_unique<scada::ReferenceType>(
        server_address_space.kTestRefTypeId, "TestRef",
        base::WideToUTF16(L"TestRef")));

    /*for (auto& node : server_address_space.nodes) {
      EXPECT_CALL(
          *this, OnNodeFetchStatusChanged(node.node_id, scada::StatusCode::Good,
                                          NodeFetchStatus::NodeOnly()));
      EXPECT_CALL(
          *this, OnNodeFetchStatusChanged(node.node_id, scada::StatusCode::Good,
                                          NodeFetchStatus::NodeAndChildren()));
    }*/

    fetcher.OnChannelOpened();
    /*fetcher.FetchNode(scada::id::RootFolder,
                      NodeFetchStatus::NodeAndChildren());*/
    Mock::VerifyAndClearExpectations(this);
  }

  MOCK_METHOD3(OnNodeFetchStatusChanged,
               void(const scada::NodeId& node_id,
                    const scada::StatusCode& status_code,
                    const NodeFetchStatus& fetch_status));

  const std::shared_ptr<Logger> logger = std::make_shared<NullLogger>();

  TestAddressSpace server_address_space;

  AddressSpaceImpl2 client_address_space{logger};

  GenericNodeFactory node_factory{client_address_space};

  AddressSpaceFetcher fetcher{AddressSpaceFetcherContext{
      logger, server_address_space, server_address_space, client_address_space,
      node_factory,
      [this](const scada::NodeId& node_id,
             const scada::Status& status,
             const NodeFetchStatus& fetch_status) {
        OnNodeFetchStatusChanged(node_id, status.code(), fetch_status);
      },
      [](const scada::ModelChangeEvent& event) {}}};
};

}  // namespace

TEST(AddressSpaceFetcher, ConfigurationLoad) {
  TestContext context;

  {
    auto* node = context.client_address_space.GetNode(scada::id::RootFolder);
    ASSERT_TRUE(node);
    EXPECT_TRUE(FindChild(*node, "TestNode1"));
    EXPECT_TRUE(FindChild(*node, "TestNode2"));
  }

  {
    auto* node = context.client_address_space.GetNode(
        context.server_address_space.kTestNode1Id);
    ASSERT_TRUE(node);
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
    ASSERT_TRUE(node);
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
                                                scada::StatusCode::Good,
                                                NodeFetchStatus::NodeOnly()));
  EXPECT_CALL(context, OnNodeFetchStatusChanged(
                           kNewNode.node_id, scada::StatusCode::Good,
                           NodeFetchStatus::NodeAndChildren()));

  context.server_address_space.CreateNode(kNewNode);

  context.fetcher.FetchNode(kNewNode.node_id,
                            NodeFetchStatus::NodeAndChildren());

  {
    auto* node = context.client_address_space.GetNode(kNewNode.node_id);
    ASSERT_TRUE(node);
    EXPECT_EQ(node->GetBrowseName(), "NewNode");
  }
}

TEST(AddressSpaceFetcher, NodeDeleted) {
  TestContext context;

  const scada::NodeId kNodeId = context.server_address_space.kTestNode1Id;
  ASSERT_TRUE(context.server_address_space.GetNode(kNodeId));

  context.server_address_space.NotifyModelChanged(
      {kNodeId, {}, scada::ModelChangeEvent::NodeDeleted});

  EXPECT_EQ(nullptr, context.client_address_space.GetNode(kNodeId));
}

TEST(AddressSpaceFetcher, NodeSemanticsChanged) {
  TestContext context;

  const scada::NodeId kNodeId = context.server_address_space.kTestNode1Id;
  const scada::QualifiedName kNewBrowseName{"NewTestNode1"};
  const scada::Variant kNewValue{"TestNode1.TestProp1.NewValue"};

  auto* node = context.server_address_space.GetNode(kNodeId);
  ASSERT_TRUE(node);

  {
    // Rename
    EXPECT_EQ("TestNode1", node->GetBrowseName());
    node->SetBrowseName(kNewBrowseName);
    // Change property
    node->SetPropertyValue(context.server_address_space.kTestProp1Id,
                           kNewValue);
  }

  context.server_address_space.NotifySemanticChanged(
      kNodeId, scada::GetTypeDefinitionId(*node));

  {
    auto* node = context.client_address_space.GetNode(kNodeId);
    ASSERT_TRUE(node);
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

  /*auto* node = context.server_address_space.GetNode(kNodeId);
  ASSERT_NE(node, nullptr);
  const auto saved_node = *node;
  context.server_address_space.DeleteNode(kNodeId);
  context.server_address_space.NotifyModelChanged(
      {kParentNodeId, {}, scada::ModelChangeEvent::ReferenceDeleted});

  EXPECT_EQ(nullptr, context.client_address_space.GetNode(kNodeId));

  // Restore reference. Expect node is refetched on client.

  EXPECT_CALL(context,
              OnNodeFetchStatusChanged(kNodeId, scada::StatusCode::Good,
                                       NodeFetchStatus::NodeOnly()));
  EXPECT_CALL(context,
              OnNodeFetchStatusChanged(kNodeId, scada::StatusCode::Good,
                                       NodeFetchStatus::NodeAndChildren()));

  context.server_address_space.nodes.emplace_back(saved_node);
  context.server_address_space.NotifyModelChanged(
      {kParentNodeId, {}, scada::ModelChangeEvent::ReferenceAdded});

  EXPECT_NE(nullptr, context.client_address_space.GetNode(kNodeId));*/
}
