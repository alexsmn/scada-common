#include "node_service/node_service.h"

#include "model/node_id_util.h"
#include "node_service/v1/test/node_service_test_context.h"

namespace v1 {

using namespace testing;

TEST(NodeServiceImpl, FetchNode) {
  NodeServiceTestContext context;

  auto node =
      context.node_service.GetNode(context.server_address_space.kTestNode2Id);
  node.Fetch(NodeFetchStatus::NodeOnly());

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.browse_name(), "TestNode2");
  EXPECT_EQ(scada::Variant{"TestNode2.TestProp1.Value"},
            node[context.server_address_space.kTestProp1Id].value());
  EXPECT_EQ(scada::Variant{"TestNode2.TestProp2.Value"},
            node[context.server_address_space.kTestProp2Id].value());
}

TEST(NodeServiceImpl, FetchUnknownNode) {
  NodeServiceTestContext context;

  const scada::NodeId kUnknownNodeId{1, 100};

  auto node = context.node_service.GetNode(kUnknownNodeId);
  node.Fetch(NodeFetchStatus::NodeOnly());
  EXPECT_TRUE(node.fetched());
  EXPECT_FALSE(node.status());
}

TEST(NodeServiceImpl, NodeAdded) {
  NodeServiceTestContext context;

  const scada::NodeState kNewNode{
      scada::NodeId{"NewNodeId", 0},
      scada::NodeClass::Object,
      context.server_address_space.kTestTypeId,
      scada::id::RootFolder,
      scada::id::Organizes,
      scada::NodeAttributes{}.set_browse_name("NewNode"),
  };

  context.server_address_space.CreateNode(kNewNode);

  const scada::ModelChangeEvent event{kNewNode.node_id,
                                      kNewNode.type_definition_id,
                                      scada::ModelChangeEvent::NodeAdded};
  EXPECT_CALL(context.node_observer, OnModelChanged(event));
  context.view_events->OnModelChanged(event);

  Mock::VerifyAndClearExpectations(&context.node_observer);

  // With smart fetch only:
  // EXPECT_CALL(context.node_observer, OnNodeFetched(kNewNode.node_id, false));

  auto node = context.node_service.GetNode(kNewNode.node_id);
  node.Fetch(NodeFetchStatus::NodeOnly());
  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.browse_name(), "NewNode");
}

TEST(NodeServiceImpl, NodeDeleted) {
  NodeServiceTestContext context;

  const scada::NodeId kNodeId = context.server_address_space.kTestNode1Id;
  ASSERT_TRUE(context.server_address_space.GetNode(kNodeId));

  const scada::ModelChangeEvent event{
      kNodeId, {}, scada::ModelChangeEvent::NodeDeleted};
  EXPECT_CALL(context.node_observer, OnModelChanged(event));
  context.view_events->OnModelChanged(event);

  context.server_address_space.DeleteNode(kNodeId);

  Mock::VerifyAndClearExpectations(&context.node_observer);

  auto node = context.node_service.GetNode(kNodeId);
  node.Fetch(NodeFetchStatus::NodeOnly());
  EXPECT_TRUE(node.fetched());
  EXPECT_FALSE(node.status());
}

TEST(NodeServiceImpl, NodeSemanticsChanged) {
  NodeServiceTestContext context;

  const scada::NodeId kNodeId = context.server_address_space.kTestNode1Id;
  const scada::LocalizedText kNewDisplayName{
      base::WideToUTF16(L"NewTestNode1")};
  const scada::Variant kNewValue{"TestNode1.TestProp1.NewValue"};

  auto node = context.node_service.GetNode(kNodeId);
  node.Fetch(NodeFetchStatus::NodeOnly());

  EXPECT_CALL(context.node_observer, OnNodeSemanticChanged(kNodeId))
      .Times(AtLeast(1));
  context.server_address_space.ModifyNode(
      kNodeId, scada::NodeAttributes{}.set_display_name(kNewDisplayName),
      {{context.server_address_space.kTestProp1Id, kNewValue}});
  context.view_events->OnNodeSemanticsChanged(
      scada::SemanticChangeEvent{kNodeId});

  EXPECT_EQ(kNewDisplayName, node.display_name());
  EXPECT_EQ(kNewValue, node[context.server_address_space.kTestProp1Id].value());
}

TEST(NodeServiceImpl, DISABLED_DeleteNodeByDeletionOfParentReference) {
  NodeServiceTestContext context;

  const scada::NodeId kParentNodeId = context.server_address_space.kTestNode3Id;
  const scada::NodeId kNodeId = context.server_address_space.kTestNode4Id;

  auto* parent_node = context.server_address_space.GetNode(kParentNodeId);
  ASSERT_TRUE(parent_node);

  /*
    scada::NodeState saved_node_state;

    // Works only if parent was pre-fetched.
    context.node_service.GetNode(parent_node->id())
        .Fetch(NodeFetchStatus::NodeOnly());

    {
      auto* node = context.server_address_space.GetNode(kNodeId);
      ASSERT_NE(node, nullptr);
      saved_node_state = *node;

      // Delete node, but notify only deleted reference deletion.
      // Expect node is deleted on client.

      EXPECT_CALL(context.node_observer,
                  OnModelChanged(scada::ModelChangeEvent{
                      node->node_id, {},
    scada::ModelChangeEvent::NodeDeleted}));

      context.server_address_space.RemoveNode(kNodeId);
      context.server_address_space.NotifyModelChanged(
          {parent_node->id(), scada::GetTypeDefinitionId(*parent_node),
           scada::ModelChangeEvent::ReferenceDeleted});

      Mock::VerifyAndClearExpectations(&context.node_observer);
    }

    {
      auto node = context.node_service.GetNode(kNodeId);
      node.Fetch(NodeFetchStatus::NodeOnly());
      EXPECT_TRUE(node.fetched());
      EXPECT_FALSE(node.status());
    }

    // Restore reference. Expect node is refetched on client.

    EXPECT_CALL(context.node_observer,
                OnModelChanged(scada::ModelChangeEvent{
                    saved_node_state.node_id,
    saved_node_state.type_definition_id, scada::ModelChangeEvent::NodeAdded}));

    context.server_address_space.nodes.emplace_back(saved_node_state);
    context.server_address_space.NotifyModelChanged(
        {saved_node_state.node_id, saved_node_state.type_definition_id,
         scada::ModelChangeEvent::NodeAdded});

    Mock::VerifyAndClearExpectations(&context.node_observer);

    EXPECT_CALL(context.node_observer,
                OnNodeFetched(saved_node_state.node_id, _));

    auto node = context.node_service.GetNode(saved_node_state.node_id);
    node.Fetch(NodeFetchStatus::NodeOnly());
    EXPECT_TRUE(node.fetched());
    EXPECT_TRUE(node.status());
    EXPECT_EQ(node.display_name(), saved_node_state.attributes.display_name);*/
}

TEST(NodeServiceImpl, DISABLED_ReferencedNodeDeletionAndRestoration) {
  NodeServiceTestContext context;

  /*const scada::NodeId kNodeId = context.server_address_space.kTestNode1Id;
  scada::NodeState saved_node_state;
  {
    auto* node = context.server_address_space.GetNode(kNodeId);
    ASSERT_NE(node, nullptr);
    saved_node_state = *node;
  }

  auto node = context.node_service.GetNode(kNodeId);
  node.Fetch(NodeFetchStatus::NodeOnly());
  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());

  EXPECT_CALL(context.node_observer,
              OnModelChanged(scada::ModelChangeEvent{
                  kNodeId, {}, scada::ModelChangeEvent::NodeDeleted}));

  context.server_address_space.DeleteNode(kNodeId);
  context.server_address_space.NotifyModelChanged(
      {kNodeId, {}, scada::ModelChangeEvent::NodeDeleted});

  Mock::VerifyAndClearExpectations(&context.node_service);

  EXPECT_TRUE(node.fetched());
  EXPECT_FALSE(node.status());
  EXPECT_EQ(node.node_id(), saved_node_state.node_id);
  EXPECT_EQ(node.display_name(),
            scada::ToLocalizedText(NodeIdToScadaString(kNodeId)));

  EXPECT_CALL(context.node_observer,
              OnModelChanged(scada::ModelChangeEvent{
                  saved_node_state.node_id, saved_node_state.type_definition_id,
                  scada::ModelChangeEvent::NodeAdded}));
  EXPECT_CALL(context.node_observer,
              OnNodeFetched(saved_node_state.node_id, false));
  EXPECT_CALL(context.node_observer,
              OnNodeFetched(saved_node_state.node_id, true));

  // Restore node.
  context.server_address_space.nodes.emplace_back(saved_node_state);
  context.server_address_space.NotifyModelChanged(
      {saved_node_state.node_id, saved_node_state.type_definition_id,
       scada::ModelChangeEvent::NodeAdded});

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.node_id(), kNodeId);
  EXPECT_EQ(node.display_name(), saved_node_state.attributes.display_name);*/
}

TEST(NodeServiceImpl, DISABLED_FetchProperty) {
  NodeServiceTestContext context;

  auto node =
      context.node_service.GetNode(context.server_address_space.kTestNode1Id);
  auto prop = node[context.server_address_space.kTestProp1Id];
}

}  // namespace v1
