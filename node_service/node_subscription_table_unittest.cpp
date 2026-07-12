#include "node_service/node_subscription_table.h"

#include "scada/event.h"

#include <gtest/gtest.h>

namespace {

scada::NodeId Id(uint32_t i) {
  return scada::NodeId{i, 1};
}

// A node with no subscription has no entry and emitting for it is a no-op.
TEST(NodeSubscriptionTableTest, UnsubscribedNodeHasNoEntry) {
  NodeSubscriptionTable table;
  EXPECT_EQ(table.entry_count(), 0u);

  // No subscribers: emitting must not crash and must create nothing.
  table.EmitNodeSemanticChanged(Id(1));
  table.EmitNodeFetched({Id(1), {}});
  EXPECT_EQ(table.entry_count(), 0u);
}

// A subscription materializes exactly one entry and delivers matching events.
TEST(NodeSubscriptionTableTest, DeliversToSubscribedNodeOnly) {
  NodeSubscriptionTable table;

  int calls = 0;
  scada::NodeId last;
  auto connection =
      table.SubscribeNodeSemanticChanged(Id(1), [&](const scada::NodeId& id) {
        ++calls;
        last = id;
      });

  EXPECT_EQ(table.entry_count(), 1u);

  // Event for another node is not delivered.
  table.EmitNodeSemanticChanged(Id(2));
  EXPECT_EQ(calls, 0);

  // Event for the subscribed node is delivered.
  table.EmitNodeSemanticChanged(Id(1));
  EXPECT_EQ(calls, 1);
  EXPECT_EQ(last, Id(1));
}

// Releasing the last connection for a node erases its entry; a later emit is a
// no-op and the map does not retain the node.
TEST(NodeSubscriptionTableTest, LastReleaseErasesEntry) {
  NodeSubscriptionTable table;

  int calls = 0;
  {
    auto connection = table.SubscribeNodeSemanticChanged(
        Id(1), [&](const scada::NodeId&) { ++calls; });
    EXPECT_EQ(table.entry_count(), 1u);
  }

  // The scoped_connection was released: entry is gone, emit reaches nobody.
  EXPECT_EQ(table.entry_count(), 0u);
  table.EmitNodeSemanticChanged(Id(1));
  EXPECT_EQ(calls, 0);
}

// Multiple subscribers to the same node share one entry; it survives until the
// last connection drops.
TEST(NodeSubscriptionTableTest, EntrySharedUntilLastSubscriber) {
  NodeSubscriptionTable table;

  int a = 0, b = 0;
  auto first = table.SubscribeNodeSemanticChanged(
      Id(1), [&](const scada::NodeId&) { ++a; });
  auto second = table.SubscribeNodeSemanticChanged(
      Id(1), [&](const scada::NodeId&) { ++b; });

  // One shared entry for the two subscriptions.
  EXPECT_EQ(table.entry_count(), 1u);

  table.EmitNodeSemanticChanged(Id(1));
  EXPECT_EQ(a, 1);
  EXPECT_EQ(b, 1);

  // Dropping one subscription keeps the entry alive for the other.
  first.disconnect();
  EXPECT_EQ(table.entry_count(), 1u);
  table.EmitNodeSemanticChanged(Id(1));
  EXPECT_EQ(a, 1);
  EXPECT_EQ(b, 2);

  // Dropping the last erases the entry.
  second.disconnect();
  EXPECT_EQ(table.entry_count(), 0u);
}

// Each event channel is independent and routed by node id.
TEST(NodeSubscriptionTableTest, AllChannelsRoute) {
  NodeSubscriptionTable table;

  int model = 0, semantic = 0, fetched = 0, state = 0;
  auto c1 = table.SubscribeModelChanged(
      Id(1), [&](const scada::ModelChangeEvent&) { ++model; });
  auto c2 = table.SubscribeNodeSemanticChanged(
      Id(1), [&](const scada::NodeId&) { ++semantic; });
  auto c3 = table.SubscribeNodeFetched(
      Id(1), [&](const NodeFetchedEvent&) { ++fetched; });
  auto c4 = table.SubscribeNodeStateChanged(
      Id(1), [&](const NodeStateChangedEvent&) { ++state; });

  EXPECT_EQ(table.entry_count(), 1u);

  table.EmitModelChanged(scada::ModelChangeEvent{
      Id(1), scada::NodeId{}, scada::ModelChangeEvent::ReferenceAdded});
  table.EmitNodeSemanticChanged(Id(1));
  table.EmitNodeFetched({Id(1), {}});
  table.EmitNodeStateChanged(
      {Id(1), std::make_shared<const scada::NodeState>(), NodeFetchStatus{}});

  EXPECT_EQ(model, 1);
  EXPECT_EQ(semantic, 1);
  EXPECT_EQ(fetched, 1);
  EXPECT_EQ(state, 1);
}

// A callback that releases its own connection during emission is safe (the
// entry is kept alive for the duration of the emit).
TEST(NodeSubscriptionTableTest, SelfDisconnectDuringEmitIsSafe) {
  NodeSubscriptionTable table;

  int calls = 0;
  auto connection = std::make_shared<boost::signals2::scoped_connection>();
  *connection = table.SubscribeNodeSemanticChanged(
      Id(1), [&, connection](const scada::NodeId&) {
        ++calls;
        connection->disconnect();
      });

  table.EmitNodeSemanticChanged(Id(1));
  EXPECT_EQ(calls, 1);

  // The self-disconnect removed the only subscription.
  EXPECT_EQ(table.entry_count(), 0u);
  table.EmitNodeSemanticChanged(Id(1));
  EXPECT_EQ(calls, 1);
}

}  // namespace
