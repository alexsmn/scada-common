#include "node_service/address_space/address_space_fetcher.h"

#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/test/test_address_space.h"
#include "address_space/variable.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_executor.h"
#include "core/monitored_item_service_mock.h"

#include <boost/asio/io_context.hpp>
#include <gmock/gmock.h>

using namespace testing;

namespace {

struct TestContext {
  TestContext() {
    fetcher->OnChannelOpened();
    /*fetcher->FetchNode(scada::id::RootFolder,
                      NodeFetchStatus::NodeAndChildren());*/
    Mock::VerifyAndClearExpectations(this);
  }

  MOCK_METHOD3(OnNodeFetchStatusChanged,
               void(const scada::NodeId& node_id,
                    const scada::StatusCode& status_code,
                    const NodeFetchStatus& fetch_status));

  const std::shared_ptr<Logger> logger = std::make_shared<NullLogger>();
  boost::asio::io_context io_context;
  const std::shared_ptr<TestExecutor> executor =
      std::make_shared<TestExecutor>();

  TestAddressSpace server_address_space;

  NiceMock<scada::MockMonitoredItemService> monitored_item_service;

  AddressSpaceImpl2 client_address_space{logger};

  GenericNodeFactory node_factory{client_address_space};

  const std::shared_ptr<AddressSpaceFetcher> fetcher =
      AddressSpaceFetcher::Create({AddressSpaceFetcherContext{
          io_context, executor, server_address_space, server_address_space,
          monitored_item_service, client_address_space, node_factory,
          [this](const scada::NodeId& node_id,
                 const scada::Status& status,
                 const NodeFetchStatus& fetch_status) {
            OnNodeFetchStatusChanged(node_id, status.code(), fetch_status);
          },
          [](const scada::ModelChangeEvent& event) {}}});
};

}  // namespace

TEST(AddressSpaceFetcher, DISABLED_ConfigurationLoad) {
  NiceMock<TestContext> context;

  context.fetcher->FetchNode(scada::id::RootFolder,
                             NodeFetchStatus::NodeAndChildren());

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
    EXPECT_EQ(scada::Variant{"TestNode1.TestProp1.Value"},
              scada::GetPropertyValue(
                  *node, context.server_address_space.kTestProp1Id));
    EXPECT_TRUE(scada::GetPropertyValue(
                    *node, context.server_address_space.kTestProp2Id)
                    .is_null());
  }

  {
    auto* node = context.client_address_space.GetNode(
        context.server_address_space.kTestNode2Id);
    ASSERT_TRUE(node);
    EXPECT_EQ(node->GetBrowseName(), "TestNode2");
    EXPECT_EQ(scada::Variant{"TestNode2.TestProp1.Value"},
              scada::GetPropertyValue(
                  *node, context.server_address_space.kTestProp1Id));
    EXPECT_EQ(scada::Variant{"TestNode2.TestProp2.Value"},
              scada::GetPropertyValue(
                  *node, context.server_address_space.kTestProp2Id));
  }
}

TEST(AddressSpaceFetcher, DISABLED_NodeAdded) {
  NiceMock<TestContext> context;

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

  context.fetcher->FetchNode(kNewNode.node_id,
                             NodeFetchStatus::NodeAndChildren());

  {
    auto* node = context.client_address_space.GetNode(kNewNode.node_id);
    ASSERT_TRUE(node);
    EXPECT_EQ(node->GetBrowseName(), "NewNode");
  }
}

TEST(AddressSpaceFetcher, NodeDeleted) {
  NiceMock<TestContext> context;

  const scada::NodeId kNodeId = context.server_address_space.kTestNode1Id;
  ASSERT_TRUE(context.server_address_space.GetNode(kNodeId));

  // context.server_address_space.NotifyEvent(scada::ModelChangeEvent{
  //     kNodeId, {}, scada::ModelChangeEvent::NodeDeleted});

  EXPECT_EQ(nullptr, context.client_address_space.GetNode(kNodeId));
}

TEST(AddressSpaceFetcher, DISABLED_NodeSemanticsChanged) {
  NiceMock<TestContext> context;

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
    scada::SetPropertyValue(*node, context.server_address_space.kTestProp1Id,
                            kNewValue);
  }

  // context.server_address_space.NotifyEvent(scada::SemanticChangeEvent{kNodeId});

  {
    auto* node = context.client_address_space.GetNode(kNodeId);
    ASSERT_TRUE(node);
    EXPECT_EQ(kNewBrowseName, node->GetBrowseName());
    EXPECT_EQ(kNewValue, scada::GetPropertyValue(
                             *node, context.server_address_space.kTestProp1Id));
  }
}

TEST(AddressSpaceFetcher, DeleteNodeByDeletionOfParentReference) {
  NiceMock<TestContext> context;

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
