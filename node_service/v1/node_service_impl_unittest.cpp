// WARNING: This is being migrated to common NodeService UTs.

#include "node_service/node_service.h"

#include "model/node_id_util.h"
#include "node_service/v1/test/node_service_test_context.h"

namespace v1 {

using namespace testing;

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
