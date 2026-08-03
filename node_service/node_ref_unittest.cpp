#include "node_service/node_ref.h"

#include "node_service/test/fake_node_service.h"

#include "base/async_completion.h"
#include "base/awaitable.h"
#include "common/node_state.h"
#include "scada/standard_node_ids.h"

#include <boost/asio/io_context.hpp>

#include <gtest/gtest.h>

#include <optional>
#include <unordered_set>
#include <vector>

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

  EXPECT_EQ(std::hash<NodeRef>{}(ref), std::hash<scada::NodeId>{}(Id(42)));

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
  service.Add(scada::NodeState{
      .node_id = Id(1),
      .attributes = {.browse_name = scada::QualifiedName{"TheName"},
                     .display_name = scada::LocalizedText{u"Displayed"}}});

  NodeRef ref = service.GetNode(Id(1));
  EXPECT_EQ(ref.browse_name(), scada::QualifiedName{"TheName"});
  EXPECT_EQ(ref.display_name(), scada::LocalizedText{u"Displayed"});
  EXPECT_EQ(ref.node_id(), Id(1));
}

TEST(NodeRefTest, GraphNavigationForwardsToService) {
  FakeNodeService service;
  service.Add(scada::NodeState{.node_id = Id(10)});  // parent
  service.Add(scada::NodeState{
      .node_id = Id(11),
      .type_definition_id = Id(100),
      .parent_id = Id(10),
      .reference_type_id = scada::id::Organizes,
      .references = {{scada::id::HasComponent, true, Id(12)}}});
  service.Add(scada::NodeState{.node_id = Id(12)});   // component
  service.Add(scada::NodeState{.node_id = Id(100)});  // type def

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
  auto connection =
      ref.SubscribeNodeStateChanged([](const NodeStateChangedEvent&) {});
  EXPECT_FALSE(connection.connected());
}

// --- Subscriptions --------------------------------------------------------

TEST(NodeRefTest, SubscribeNodeStateChangedReceivesEmittedEvents) {
  FakeNodeService service;
  service.Add(scada::NodeState{.node_id = Id(1)});

  NodeRef node = service.GetNode(Id(1));

  int calls = 0;
  scada::NodeId seen;
  auto connection =
      node.SubscribeNodeStateChanged([&](const NodeStateChangedEvent& event) {
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

// --- Reference-type matching ----------------------------------------------

TEST(NodeRefTest, BaseReferenceTypeQueryMatchesStandardSubtypes) {
  // A HierarchicalReferences query must match Organizes / HasComponent edges
  // without the ns0 ReferenceType nodes being registered — standard reference
  // types resolve statically, as in the production services.
  FakeNodeService service;
  service.Add(scada::NodeState{.node_id = Id(10)});
  service.Add(scada::NodeState{.node_id = Id(11),
                               .parent_id = Id(10),
                               .reference_type_id = scada::id::Organizes});
  service.Add(scada::NodeState{.node_id = Id(12),
                               .parent_id = Id(10),
                               .reference_type_id = scada::id::HasComponent});

  NodeRef parent = service.GetNode(Id(10));
  auto children = parent.targets(scada::id::HierarchicalReferences);
  EXPECT_EQ(children.size(), 2u);

  // A query for a sibling branch of the hierarchy still discriminates.
  EXPECT_EQ(parent.targets(scada::id::Organizes).size(), 1u);
}

TEST(NodeRefTest, ChildLookupResolvesThroughBaseReferenceType) {
  FakeNodeService service;
  service.Add(scada::NodeState{.node_id = Id(10)});
  service.Add(scada::NodeState{
      .node_id = Id(11),
      .parent_id = Id(10),
      .reference_type_id = scada::id::HasComponent,
      .attributes = {.browse_name = scada::QualifiedName{"Child"}}});

  NodeRef child = service.GetNode(Id(10))[scada::QualifiedName{"Child"}];
  ASSERT_TRUE(child);
  EXPECT_EQ(child.node_id(), Id(11));
}

// --- Fetch control --------------------------------------------------------

TEST(NodeRefTest, FetchStatusAndStatusAreControllablePerNode) {
  FakeNodeService service;
  service.Add(scada::NodeState{.node_id = Id(1)});
  service.Add(scada::NodeState{.node_id = Id(2)});

  // Registered nodes default to fully fetched and Good.
  EXPECT_TRUE(service.GetNode(Id(1)).fetched());
  EXPECT_TRUE(service.GetNode(Id(1)).status());

  service.SetFetchStatus(Id(1), NodeFetchStatus::NodeOnly);
  service.SetStatus(Id(1), scada::StatusCode::Bad_WrongNodeId);

  EXPECT_EQ(service.GetFetchStatus(Id(1)), NodeFetchStatus::NodeOnly);
  EXPECT_FALSE(service.GetNode(Id(1)).status());
  // Other nodes are unaffected.
  EXPECT_TRUE(service.GetNode(Id(2)).fetched());
}

TEST(NodeRefTest, FetchRequestsAreRecorded) {
  FakeNodeService service;
  NodeRef node = service.Add(scada::NodeState{.node_id = Id(1)});

  EXPECT_TRUE(service.fetch_requests(Id(1)).empty());

  node.StartFetch(NodeFetchStatus::NodeOnly);
  node.StartFetch(NodeFetchStatus::NodeAndChildren);

  EXPECT_EQ(service.fetch_requests(Id(1)),
            (std::vector{NodeFetchStatus::NodeOnly,
                         NodeFetchStatus::NodeAndChildren}));

  service.ClearFetchRequests();
  EXPECT_TRUE(service.fetch_requests(Id(1)).empty());
}

TEST(NodeRefTest, FetchHandlerCanSuspendAndPublishStatusOnResume) {
  boost::asio::io_context io_context;

  FakeNodeService service;
  NodeRef node = service.Add(scada::NodeState{.node_id = Id(1)});
  service.SetFetchStatus(Id(1), NodeFetchStatus::None);

  std::optional<scada::base::AsyncCompletion> gate;
  service.SetFetchHandler(
      Id(1), [&](const NodeFetchStatus&) -> Awaitable<void> {
        co_await gate->Wait();
        service.SetFetchStatus(Id(1), NodeFetchStatus::NodeOnly);
      });

  bool fetch_returned = false;
  RunAwaitable(io_context, [&]() -> Awaitable<void> {
    gate.emplace(io_context.get_executor());
    // Release the gate from a second coroutine so the fetch really suspends.
    CoSpawn(io_context.get_executor(), [&]() -> Awaitable<void> {
      EXPECT_FALSE(node.fetched());
      gate->Complete();
      co_return;
    });

    co_await node.Fetch(NodeFetchStatus::NodeOnly);
    fetch_returned = true;
  });

  EXPECT_TRUE(fetch_returned);
  EXPECT_TRUE(node.fetched());
  EXPECT_EQ(service.fetch_requests(Id(1)),
            (std::vector{NodeFetchStatus::NodeOnly}));
}

// --- Aggregates -----------------------------------------------------------

TEST(NodeRefTest, PropertiesAreMaterializedAsAggregates) {
  FakeNodeService service;
  service.Add(scada::NodeState{.node_id = Id(1),
                               .type_definition_id = Id(100),
                               .properties = {{Id(200), scada::Variant{42}}}});

  NodeRef property = service.GetNode(Id(1))[Id(200)];
  ASSERT_TRUE(property);
  EXPECT_EQ(property.attribute(scada::AttributeId::Value), scada::Variant{42});

  // An unknown declaration resolves to nothing rather than a live cursor.
  EXPECT_FALSE(service.GetNode(Id(1))[Id(201)]);
}

}  // namespace
