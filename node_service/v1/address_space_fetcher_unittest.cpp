#include "node_service/v1/address_space_fetcher.h"

#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/test/test_address_space.h"
#include "address_space/test/test_matchers.h"
#include "address_space/variable.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_executor.h"
#include "core/monitored_item_service_mock.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

class AddressSpaceFetcherTest : public Test {
 public:
  AddressSpaceFetcherTest() {
    address_space_fetcher_->OnChannelOpened();
    /*address_space_fetcher_->FetchNode(scada::id::RootFolder,
                      NodeFetchStatus::NodeAndChildren());*/
    Mock::VerifyAndClearExpectations(this);
  }

  StrictMock<
      MockFunction<void(base::span<const NodeFetchStatusChangedItem> items)>>
      node_fetch_status_changed_handler_;

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  TestAddressSpace server_address_space_;

  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;

  AddressSpaceImpl client_address_space_;

  GenericNodeFactory node_factory{client_address_space_};

  scada::ViewEvents* view_events = nullptr;

  const std::shared_ptr<AddressSpaceFetcher> address_space_fetcher_ =
      AddressSpaceFetcher::Create({AddressSpaceFetcherContext{
          executor_, server_address_space_, server_address_space_,
          client_address_space_, node_factory,
          [&](scada::ViewEvents& events) {
            view_events = &events;
            return std::make_unique<IViewEventsSubscription>();
          },
          node_fetch_status_changed_handler_.AsStdFunction(),
          [](const scada::ModelChangeEvent& event) {},
          [](const scada::SemanticChangeEvent& event) {}}});
};

TEST_F(AddressSpaceFetcherTest, FetchNode_NodeOnly) {
  const auto node_id = server_address_space_.kTestNode1Id;

  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(server_address_space_, Browse(_, _)).Times(AtLeast(1));

  EXPECT_CALL(
      node_fetch_status_changed_handler_,
      Call(Contains(NodeFetchStatusChangedItem{node_id, scada::StatusCode::Good,
                                               NodeFetchStatus::NodeOnly()})));

  address_space_fetcher_->FetchNode(node_id, NodeFetchStatus::NodeOnly());
}

TEST_F(AddressSpaceFetcherTest, FetchNode_NodeAndChildren_WhenReadIsDelayed) {
  // When read is delayed, NodeChildrenFetcher completes first before the node
  // is created.

  const auto node_id = server_address_space_.kTestNode1Id;

  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(server_address_space_, Browse(_, _)).Times(AtLeast(1));

  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(Contains(NodeIs(node_id))))
      .Times(AnyNumber());

  std::pair<std::shared_ptr<const std::vector<scada::ReadValueId>>,
            scada::ReadCallback>
      pending_read;
  EXPECT_CALL(server_address_space_,
              Read(_, Pointee(Contains(NodeIs(node_id))), _))
      .WillOnce(DoAll(SaveArg<1>(&pending_read.first),
                      SaveArg<2>(&pending_read.second)))
      .WillRepeatedly(DoDefault());

  address_space_fetcher_->FetchNode(node_id,
                                    NodeFetchStatus::NodeAndChildren());

  auto& [read_inputs, read_callback] = pending_read;
  ASSERT_TRUE(read_callback);

  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(Contains(NodeFetchStatusChangedItem{
                  node_id, scada::StatusCode::Good,
                  NodeFetchStatus::NodeAndChildren()})));

  server_address_space_.attribute_service_impl.Read(nullptr, read_inputs,
                                                    read_callback);
}

TEST_F(AddressSpaceFetcherTest, DISABLED_ConfigurationLoad) {
  address_space_fetcher_->FetchNode(scada::id::RootFolder,
                                    NodeFetchStatus::NodeAndChildren());

  {
    auto* node = client_address_space_.GetNode(scada::id::RootFolder);
    ASSERT_TRUE(node);
    EXPECT_TRUE(FindChild(*node, "TestNode1"));
    EXPECT_TRUE(FindChild(*node, "TestNode2"));
  }

  {
    auto* node =
        client_address_space_.GetNode(server_address_space_.kTestNode1Id);
    ASSERT_TRUE(node);
    EXPECT_EQ(node->GetBrowseName(), "TestNode1");
    EXPECT_EQ(
        scada::Variant{"TestNode1.TestProp1.Value"},
        scada::GetPropertyValue(*node, server_address_space_.kTestProp1Id));
    EXPECT_TRUE(
        scada::GetPropertyValue(*node, server_address_space_.kTestProp2Id)
            .is_null());
  }

  {
    auto* node =
        client_address_space_.GetNode(server_address_space_.kTestNode2Id);
    ASSERT_TRUE(node);
    EXPECT_EQ(node->GetBrowseName(), "TestNode2");
    EXPECT_EQ(
        scada::Variant{"TestNode2.TestProp1.Value"},
        scada::GetPropertyValue(*node, server_address_space_.kTestProp1Id));
    EXPECT_EQ(
        scada::Variant{"TestNode2.TestProp2.Value"},
        scada::GetPropertyValue(*node, server_address_space_.kTestProp2Id));
  }
}

TEST_F(AddressSpaceFetcherTest, DISABLED_NodeAdded) {
  const scada::NodeState new_node_state{
      scada::NodeId{"NewNodeId", 0},
      scada::NodeClass::Object,
      server_address_space_.kTestTypeId,
      scada::id::RootFolder,
      scada::id::Organizes,
      scada::NodeAttributes{}.set_browse_name("NewNode"),
  };

  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(Contains(NodeFetchStatusChangedItem{
                  new_node_state.node_id, scada::StatusCode::Good,
                  NodeFetchStatus::NodeOnly()})));
  EXPECT_CALL(node_fetch_status_changed_handler_,
              Call(Contains(NodeFetchStatusChangedItem{
                  new_node_state.node_id, scada::StatusCode::Good,
                  NodeFetchStatus::NodeAndChildren()})));

  server_address_space_.CreateNode(new_node_state);

  address_space_fetcher_->FetchNode(new_node_state.node_id,
                                    NodeFetchStatus::NodeAndChildren());

  {
    auto* node = client_address_space_.GetNode(new_node_state.node_id);
    ASSERT_TRUE(node);
    EXPECT_EQ(node->GetBrowseName(), "NewNode");
  }
}

TEST_F(AddressSpaceFetcherTest, NodeDeleted) {
  const scada::NodeId kNodeId = server_address_space_.kTestNode1Id;
  ASSERT_TRUE(server_address_space_.GetNode(kNodeId));

  view_events->OnModelChanged(scada::ModelChangeEvent{
      kNodeId, {}, scada::ModelChangeEvent::NodeDeleted});

  EXPECT_FALSE(client_address_space_.GetNode(kNodeId));
}

TEST_F(AddressSpaceFetcherTest, NodeSemanticsChanged) {
  const scada::NodeId kNodeId = server_address_space_.kTestNode1Id;
  const scada::QualifiedName kNewBrowseName{"NewTestNode1"};
  const scada::Variant kNewValue{"TestNode1.TestProp1.NewValue"};

  auto* node = server_address_space_.GetMutableNode(kNodeId);
  ASSERT_TRUE(node);

  EXPECT_CALL(server_address_space_, Read(_, _, _)).Times(AtLeast(1));
  EXPECT_CALL(server_address_space_, Browse(_, _)).Times(AtLeast(1));
  EXPECT_CALL(node_fetch_status_changed_handler_, Call(_));

  address_space_fetcher_->FetchNode(kNodeId, NodeFetchStatus::NodeOnly());

  {
    // Rename
    EXPECT_EQ("TestNode1", node->GetBrowseName());
    node->SetBrowseName(kNewBrowseName);
    // Change property
    scada::SetPropertyValue(*node, server_address_space_.kTestProp1Id,
                            kNewValue);
  }

  view_events->OnNodeSemanticsChanged(scada::SemanticChangeEvent{kNodeId});

  {
    auto* client_node = client_address_space_.GetNode(kNodeId);
    ASSERT_TRUE(node);
    EXPECT_EQ(kNewBrowseName, client_node->GetBrowseName());
    EXPECT_EQ(kNewValue, scada::GetPropertyValue(
                             *client_node, server_address_space_.kTestProp1Id));
  }
}

TEST_F(AddressSpaceFetcherTest,
       DISABLED_DeleteNodeByDeletionOfParentReference) {
  const scada::NodeId kParentNodeId = server_address_space_.kTestNode3Id;
  const scada::NodeId kNodeId = server_address_space_.kTestNode4Id;

  // Delete node, but notify only deleted reference deletion.
  // Expect node is deleted on client.

  /*auto* node = server_address_space_.GetNode(kNodeId);
  ASSERT_NE(node, nullptr);
  const auto saved_node = *node;
  server_address_space_.DeleteNode(kNodeId);
  server_address_space_.NotifyModelChanged(
      {kParentNodeId, {}, scada::ModelChangeEvent::ReferenceDeleted});

  EXPECT_EQ(nullptr, client_address_space_.GetNode(kNodeId));

  // Restore reference. Expect node is refetched on client.

  EXPECT_CALL(*this,
              OnNodeFetchStatusChanged(kNodeId, scada::StatusCode::Good,
                                       NodeFetchStatus::NodeOnly()));
  EXPECT_CALL(*this,
              OnNodeFetchStatusChanged(kNodeId, scada::StatusCode::Good,
                                       NodeFetchStatus::NodeAndChildren()));

  server_address_space_.nodes.emplace_back(saved_node);
  server_address_space_.NotifyModelChanged(
      {kParentNodeId, {}, scada::ModelChangeEvent::ReferenceAdded});

  EXPECT_NE(nullptr, client_address_space_.GetNode(kNodeId));*/
}
