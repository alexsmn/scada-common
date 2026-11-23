#include "events/condition_folder.h"

#include <gtest/gtest.h>

#include <chrono>

namespace {

scada::Event MakeNotification(scada::EventId event_id,
                              const scada::NodeId& condition_id,
                              bool active,
                              scada::Time time) {
  scada::Event event;
  event.event_id = event_id;
  event.time = time;
  event.receive_time = time;
  event.source_node_id = condition_id;
  event.condition_id = condition_id;
  event.active = active;
  return event;
}

scada::Event Acked(scada::Event event) {
  event.acked = true;
  event.acknowledged_time = event.time;
  return event;
}

TEST(ConditionFolderTest, PlainEventPassesThrough) {
  ConditionFolder folder;
  scada::Event plain;
  plain.event_id = 5;
  EXPECT_EQ(folder.Fold(plain), plain);
  EXPECT_EQ(folder.UnackedNotifications(5), std::vector<scada::EventId>{5});
}

// Once a Condition is acknowledged and back to normal its row is gone, so the
// next activation is a new row with a new identity — a fresh alarm to read.
TEST(ConditionFolderTest, ReactivationAfterTheRowLeftStartsANewRow) {
  ConditionFolder folder;
  const scada::NodeId item{7, 1};
  const scada::Time t0 = scada::Now();

  folder.Fold(Acked(MakeNotification(1, item, true, t0)));
  const scada::Event done = folder.Fold(
      Acked(MakeNotification(2, item, false, t0 + std::chrono::minutes(1))));
  EXPECT_FALSE(scada::IsRetained(done));

  const scada::Event again = folder.Fold(
      MakeNotification(3, item, true, t0 + std::chrono::minutes(2)));
  EXPECT_EQ(again.event_id, 3u);
  EXPECT_TRUE(again.active);
  EXPECT_FALSE(again.acked);
}

// A third-party server's AckedState is the Condition's: one acknowledged
// notification settles every state before it.
TEST(ConditionFolderTest, ConditionScopeAcknowledgesEveryEarlierState) {
  ConditionFolder folder{ConditionFolder::AckScope::kCondition};
  const scada::NodeId item{7, 1};
  const scada::Time t0 = scada::Now();

  folder.Fold(MakeNotification(1, item, true, t0));
  const scada::Event current = folder.Fold(
      Acked(MakeNotification(2, item, true, t0 + std::chrono::minutes(1))));
  EXPECT_TRUE(current.acked);
  EXPECT_TRUE(current.is_valid());
}

}  // namespace
