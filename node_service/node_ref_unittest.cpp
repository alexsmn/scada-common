#include "node_service/node_ref.h"

#include "node_service/test/fake_node_service.h"

#include "common/node_state.h"
#include "scada/standard_node_ids.h"

#include <gtest/gtest.h>

#include <unordered_set>

namespace {

scada::NodeId Id(scada::NumericId numeric_id) {
  return scada::NodeId{numeric_id};
}

// --- Identity -------------------------------------------------------------

TEST(NodeRefTest, DefaultRefIsNull) {
  NodeRef ref;
  EXPECT_FALSE(ref);
  EXPECT_TRUE(ref.node_id().is_null());
  EXPECT_EQ(ref.service(), nullptr);
}

TEST(NodeRefTest, NodeIdIsIntrinsicWithoutServiceRoundTrip) {
  // A cursor knows its id even for a node the service has never heard of.
  FakeNodeService service;
  NodeRef ref = service.GetNode(Id(7));
  EXPECT_TRUE(ref);
  EXPECT_EQ(ref.node_id(), Id(7));
}

TEST(NodeRefTest, EqualityAndOrderingUseNodeId) {
  FakeNodeService service;
  NodeRef a = service.GetNode(Id(1));
  NodeRef a2 = service.GetNode(Id(1));
  NodeRef b = service.GetNode(Id(2));

  EXPECT_EQ(a, a2);
  EXPECT_NE(a, b);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
}

TEST(NodeRefTest, EqualityIsByNodeIdAcrossServices) {
  // Two cursors to the same node id from different services compare equal:
  // identity is the node id, not the service pointer.
  FakeNodeService service_a;
  FakeNodeService service_b;
  EXPECT_EQ(service_a.GetNode(Id(5)), service_b.GetNode(Id(5)));
  EXPECT_NE(service_a.GetNode(Id(5)), service_b.GetNode(Id(6)));
}

TEST(NodeRefTest, HashMatchesNodeIdHashAndWorksInContainers) {
  FakeNodeService service;
  NodeRef ref = service.GetNode(Id(42));

  EXPECT_EQ(std::hash<NodeRef>{}(ref),
            std::hash<scada::NodeId>{}(Id(42)));

  std::unordered_set<NodeRef> set;
  set.insert(service.GetNode(Id(1)));
  set.insert(service.GetNode(Id(1)));  // duplicate id
  set.insert(service.GetNode(Id(2)));
  EXPECT_EQ(set.size(), 2u);
  EXPECT_TRUE(set.contains(service.GetNode(Id(1))));
}

// --- Forwarding -----------------------------------------------------------

TEST(NodeRefTest, AttributeReadsForwardToService) {
  FakeNodeService service;
  service.Add(scada::NodeState{}
                  .set_node_id(Id(1))
                  .set_browse_name(scada::QualifiedName{"TheName"})
                  .set_display_name(scada::LocalizedText{u"Displayed"}));

  NodeRef ref = service.GetNode(Id(1));
  EXPECT_EQ(ref.browse_name(), scada::QualifiedName{"TheName"});
  EXPECT_EQ(ref.display_name(), scada::LocalizedText{u"Displayed"});
  EXPECT_EQ(ref.node_id(), Id(1));
}

TEST(NodeRefTest, GraphNavigationForwardsToService) {
  FakeNodeService service;
  service.Add(scada::NodeState{}.set_node_id(Id(10)));            // parent
  service.Add(scada::NodeState{}
                  .set_node_id(Id(11))
                  .set_type_definition_id(Id(100))
                  .set_parent(scada::id::Organizes, Id(10))
                  .add_reference(scada::ReferenceDescription{
                      scada::id::HasComponent, true, Id(12)}));
  service.Add(scada::NodeState{}.set_node_id(Id(12)));           // component
  service.Add(scada::NodeState{}.set_node_id(Id(100)));          // type def

  NodeRef node = service.GetNode(Id(11));
  EXPECT_EQ(node.parent().node_id(), Id(10));
  EXPECT_EQ(node.type_definition().node_id(), Id(100));

  auto components = node.targets(scada::id::HasComponent);
  ASSERT_EQ(components.size(), 1u);
  EXPECT_EQ(components.front().node_id(), Id(12));
}

// --- Null-ref safety ------------------------------------------------------

TEST(NodeRefTest, NullRefOperationsAreSafe) {
  NodeRef ref;  // no service

  EXPECT_FALSE(ref.status());
  EXPECT_TRUE(ref.attribute(scada::AttributeId::Value).is_null());
  EXPECT_EQ(ref.node_class(), std::nullopt);
  EXPECT_FALSE(ref.data_type());
  EXPECT_FALSE(ref.parent());
  EXPECT_FALSE(ref.target(scada::id::HasComponent));
  EXPECT_TRUE(ref.targets(scada::id::HasComponent).empty());
  EXPECT_FALSE(ref[scada::QualifiedName{"x"}]);

  // Subscribing through a null ref yields an empty, harmless connection.
  auto connection = ref.SubscribeNodeStateChanged(
      [](const NodeStateChangedEvent&) {});
  EXPECT_FALSE(connection.connected());
}

// --- Subscriptions --------------------------------------------------------

TEST(NodeRefTest, SubscribeNodeStateChangedReceivesEmittedEvents) {
  FakeNodeService service;
  service.Add(scada::NodeState{}.set_node_id(Id(1)));

  NodeRef node = service.GetNode(Id(1));

  int calls = 0;
  scada::NodeId seen;
  auto connection = node.SubscribeNodeStateChanged(
      [&](const NodeStateChangedEvent& event) {
        ++calls;
        seen = event.node_id;
      });

  service.EmitNodeStateChanged(Id(1));
  EXPECT_EQ(calls, 1);
  EXPECT_EQ(seen, Id(1));

  connection.disconnect();
  service.EmitNodeStateChanged(Id(1));
  EXPECT_EQ(calls, 1);  // no longer subscribed
}

}  // namespace
