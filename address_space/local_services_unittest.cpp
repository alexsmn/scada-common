#include "address_space/address_space_impl.h"
#include "address_space/local_history_service.h"
#include "address_space/local_method_service.h"
#include "address_space/local_monitored_item_service.h"
#include "address_space/local_node_management_service.h"
#include "address_space/local_session_service.h"
#include "address_space/method_service_impl.h"
#include "address_space/test/test_address_space.h"

#include "base/test/awaitable_test.h"
#include "common/sync_attribute_service.h"
#include "scada/date_time.h"
#include "scada/monitored_item.h"
#include "scada/read_value_id.h"
#include "scada/standard_node_ids.h"
#include "scada/test/status_matchers.h"

#include <boost/json.hpp>
#include <gmock/gmock.h>
#include <variant>

namespace scada {
namespace {

TEST(LocalSessionService, LifecycleOperationsComplete) {
  TestExecutor executor;
  LocalSessionService service;

  EXPECT_NO_THROW(WaitAwaitable(executor, service.Connect({})));
  EXPECT_NO_THROW(WaitAwaitable(executor, service.Reconnect()));
  EXPECT_NO_THROW(WaitAwaitable(executor, service.Disconnect()));
}

TEST(LocalSessionService, ReportsConnectedLocalSession) {
  LocalSessionService service;

  EXPECT_TRUE(service.IsConnected());
  EXPECT_TRUE(service.HasAccessRight(AccessRight::kConfigure));
  EXPECT_TRUE(service.IsScada());
  EXPECT_EQ(service.GetUserId(), NodeId{});
  EXPECT_EQ(service.GetHostName(), "local");
  EXPECT_EQ(service.GetSessionDebugger(), nullptr);
}

TEST(LocalMethodService, CoroutineCallReturnsBadStatus) {
  TestExecutor executor;
  LocalMethodService service;

  const auto status = WaitAwaitable(
      executor,
      service.Call(id::ObjectsFolder, NodeId{1, 2}, {}, ServiceContext{}));

  EXPECT_EQ(status.status().code(), StatusCode::Bad);
}

TEST(MethodServiceImpl, CoroutineCallReturnsWrongMethodId) {
  TestExecutor executor;
  AddressSpaceImpl address_space;
  MethodServiceImpl service{{address_space}};

  const auto status = WaitAwaitable(
      executor, service.Call(NodeId{1, 2}, NodeId{2, 2}, {}, ServiceContext{}));

  EXPECT_EQ(status.status().code(), StatusCode::Bad_WrongMethodId);
}

TEST(LocalNodeManagementService, CoroutineAddNodesReturnsBadResults) {
  TestExecutor executor;
  LocalNodeManagementService service;

  auto result = WaitAwaitable(
      executor, service.AddNodes(
                    ServiceContext{},
                    {AddNodesItem{.requested_id = NodeId{1, 2},
                                  .parent_id = id::ObjectsFolder,
                                  .node_class = NodeClass::Object,
                                  .type_definition_id = id::BaseObjectType}}));

  EXPECT_THAT(result, test::StatusIs(StatusCode::Bad));
}

TEST(LocalNodeManagementService, CoroutineDeleteNodesReturnsBadResults) {
  TestExecutor executor;
  LocalNodeManagementService service;

  auto result = WaitAwaitable(
      executor,
      service.DeleteNodes(ServiceContext{},
                          {DeleteNodesItem{.node_id = NodeId{1, 2}}}));

  EXPECT_THAT(result, test::StatusIs(StatusCode::Bad));
}

TEST(LocalNodeManagementService, CoroutineAddReferencesReturnsBadResults) {
  TestExecutor executor;
  LocalNodeManagementService service;

  auto result = WaitAwaitable(
      executor, service.AddReferences(
                    ServiceContext{},
                    {AddReferencesItem{.source_node_id = id::ObjectsFolder,
                                       .reference_type_id = id::Organizes,
                                       .target_node_id = NodeId{1, 2}}}));

  EXPECT_THAT(result, test::StatusIs(StatusCode::Bad));
}

TEST(LocalNodeManagementService, CoroutineDeleteReferencesReturnsBadResults) {
  TestExecutor executor;
  LocalNodeManagementService service;

  auto result = WaitAwaitable(
      executor, service.DeleteReferences(
                    ServiceContext{},
                    {DeleteReferencesItem{.source_node_id = id::ObjectsFolder,
                                          .reference_type_id = id::Organizes,
                                          .target_node_id = NodeId{1, 2}}}));

  EXPECT_THAT(result, test::StatusIs(StatusCode::Bad));
}

// Regression: LocalMonitoredItem used to deliver a random synthetic sample;
// current-value consumers must instead receive the node's actual Value
// attribute from the backing address space, stamped with fresh timestamps.
TEST(LocalMonitoredItemService, DeliversAddressSpaceValueOnSubscribe) {
  TestExecutor executor;
  ::TestAddressSpace address_space;
  LocalMonitoredItemService service{address_space.sync_attribute_service_impl};

  ASSERT_OK_AND_ASSIGN(auto subscription,
                       service.CreateSubscription(ServiceContext{}, {}));

  const NodeId value_node_id = address_space.MakeNestedNodeId(
      address_space.kTestNode1Id, address_space.kTestProp1Id);
  auto results = WaitAwaitable(
      executor, subscription->AddItems({MonitoredItemCreateRequest{
                    .item_to_monitor = {.node_id = value_node_id,
                                        .attribute_id = AttributeId::Value},
                    .client_handle = 1}}));
  ASSERT_EQ(results.size(), 1u);
  ASSERT_TRUE(results[0].status);

  ASSERT_OK_AND_ASSIGN(auto notifications,
                       WaitAwaitable(executor, subscription->ReadNext(10)));
  ASSERT_EQ(notifications.size(), 1u);
  const auto* data_change =
      std::get_if<DataChangeNotification>(&notifications[0]);
  ASSERT_NE(data_change, nullptr);
  EXPECT_EQ(data_change->client_handle, 1u);
  EXPECT_EQ(data_change->value.status_code, StatusCode::Good);
  EXPECT_EQ(data_change->value.value, Variant{"TestNode1.TestProp1.Value"});
  EXPECT_FALSE(scada::IsNull(data_change->value.source_timestamp));
  EXPECT_FALSE(scada::IsNull(data_change->value.server_timestamp));
}

// Regression: the address space used to stamp a statically configured
// attribute value with a default-constructed scada::Time. Under std::chrono
// that is the Unix epoch (1970-01-01) — a perfectly valid instant that
// IsNull() does not recognise — rather than the kNullTime sentinel. Consumers
// that fill in a missing timestamp (LocalMonitoredItem below, and the
// client's table cells) therefore saw a "real" timestamp and rendered
// 1970-01-01 beside a live value.
TEST(SyncAttributeServiceImpl, StaticValueAttributeHasNullTimestamps) {
  ::TestAddressSpace address_space;

  const NodeId value_node_id = address_space.MakeNestedNodeId(
      address_space.kTestNode1Id, address_space.kTestProp1Id);
  const DataValue value =
      ::Read(address_space.sync_attribute_service_impl, ServiceContext{},
             ReadValueId{.node_id = value_node_id,
                         .attribute_id = AttributeId::Value});

  ASSERT_EQ(value.value, Variant{"TestNode1.TestProp1.Value"});
  EXPECT_TRUE(scada::IsNull(value.source_timestamp))
      << "source_timestamp: " << value.source_timestamp.time_since_epoch();
  EXPECT_TRUE(scada::IsNull(value.server_timestamp))
      << "server_timestamp: " << value.server_timestamp.time_since_epoch();
}

// Companion to the test above, on the delivery side: because the address
// space now reports "no timestamp" as kNullTime, LocalMonitoredItem actually
// stamps the sample with the current time instead of passing the Unix epoch
// through untouched.
TEST(LocalMonitoredItemService, StampsDeliveryTimeOnUntimestampedValue) {
  TestExecutor executor;
  ::TestAddressSpace address_space;
  LocalMonitoredItemService service{address_space.sync_attribute_service_impl};

  ASSERT_OK_AND_ASSIGN(auto subscription,
                       service.CreateSubscription(ServiceContext{}, {}));

  const Time before = scada::Now();
  const NodeId value_node_id = address_space.MakeNestedNodeId(
      address_space.kTestNode1Id, address_space.kTestProp1Id);
  auto results = WaitAwaitable(
      executor, subscription->AddItems({MonitoredItemCreateRequest{
                    .item_to_monitor = {.node_id = value_node_id,
                                        .attribute_id = AttributeId::Value},
                    .client_handle = 1}}));
  ASSERT_EQ(results.size(), 1u);
  ASSERT_TRUE(results[0].status);

  ASSERT_OK_AND_ASSIGN(auto notifications,
                       WaitAwaitable(executor, subscription->ReadNext(10)));
  ASSERT_EQ(notifications.size(), 1u);
  const auto* data_change =
      std::get_if<DataChangeNotification>(&notifications[0]);
  ASSERT_NE(data_change, nullptr);

  // Both timestamps must be the delivery time. `>= before` is what fails on
  // the pre-fix code: the Unix epoch is non-null, so the stamping was skipped
  // and the sample arrived dated 1970-01-01.
  EXPECT_GE(data_change->value.source_timestamp, before)
      << "source_timestamp: "
      << data_change->value.source_timestamp.time_since_epoch();
  EXPECT_GE(data_change->value.server_timestamp, before)
      << "server_timestamp: "
      << data_change->value.server_timestamp.time_since_epoch();
}

TEST(LocalHistoryService, CoroutineHistoryReadRawReturnsGeneratedProfile) {
  TestExecutor executor;
  LocalHistoryService service;
  const NodeId node_id{7, 2};
  service.SetRawProfile(node_id, 42.0);

  auto result =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = node_id,
                                  .from = scada::Now() - std::chrono::hours(1),
                                  .to = scada::Now()}));

  ASSERT_TRUE(result.ok()) << result.status();
  ASSERT_EQ(result->values.size(), 48u);
  EXPECT_EQ(result->values.front().status_code, StatusCode::Good);
  EXPECT_FALSE(result->values.front().value.is_null());
}

// The synthesized series spans the requested window, so a narrow range (a
// table row's 1 h sparkline window) still reads a fully-populated series
// instead of the tail of a fixed 30-minute-spaced day.
TEST(LocalHistoryService, GeneratedProfileSpansTheRequestedRange) {
  TestExecutor executor;
  LocalHistoryService service;
  const NodeId node_id{7, 2};
  service.SetRawProfile(node_id, 42.0);

  const auto to = scada::Now();
  const auto from = to - std::chrono::hours(1);
  auto result =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = node_id, .from = from, .to = to}));

  ASSERT_TRUE(result.ok()) << result.status();
  ASSERT_EQ(result->values.size(), 48u);
  // All 48 points fall inside [from, to], evenly spaced, ending at `to`.
  EXPECT_GE(result->values.front().source_timestamp, from);
  EXPECT_EQ(result->values.back().source_timestamp, to - (to - from) / 48);
}

// A read whose upper bound is the "current-only" sentinel (Time::Max, used
// by live consumers that want the latest sample rather than a finite window)
// must anchor the synthesized series to now, not to Max. Anchoring to Max
// spread the 48 points across ~285,000 years, so every point but the first
// fell outside any real query window and the series read flat (the trend and
// its value grid showed constant lines with Min == Max == Average).
TEST(LocalHistoryService, UnboundedEndAnchorsToNow) {
  TestExecutor executor;
  LocalHistoryService service;
  const NodeId node_id{8, 2};
  service.SetRawProfile(node_id, 42.0);

  const auto from = scada::Now() - std::chrono::hours(1);
  auto result = WaitAwaitable(
      executor, service.HistoryReadRaw(HistoryReadRawDetails{
                    .node_id = node_id, .from = from, .to = scada::kMaxTime}));

  ASSERT_TRUE(result.ok()) << result.status();
  ASSERT_EQ(result->values.size(), 48u);
  // The newest point is within a day of now, not centuries into the future.
  const auto now = scada::Now();
  EXPECT_LT(result->values.back().source_timestamp, now + std::chrono::days(1));
  EXPECT_GT(result->values.back().source_timestamp, now - std::chrono::days(1));
}

// A window far wider than the data is meant to cover must not stretch the
// series to fit it. The trend probes for its earliest available sample to fill
// the left edge of the plot, which asks for everything from the epoch to now;
// spreading 48 points across that put them ~14 months apart, so every point
// but the last fell outside the 24 h the graph was displaying. The plot came
// out empty while the axes, limit bands and legend were all correct, which
// read as a rendering quirk rather than as bad data.
TEST(LocalHistoryService, VeryWideRangeStillYieldsADayOfSamples) {
  TestExecutor executor;
  LocalHistoryService service;
  const NodeId node_id{9, 2};
  service.SetRawProfile(node_id, 42.0);

  const auto to = scada::Now();
  auto result = WaitAwaitable(
      executor, service.HistoryReadRaw(HistoryReadRawDetails{
                    .node_id = node_id, .from = scada::Time{}, .to = to}));

  ASSERT_TRUE(result.ok()) << result.status();
  ASSERT_EQ(result->values.size(), 48u);
  // Capped at the default 30-minute spacing, so the series spans a day ending
  // at `to` rather than reaching back to the epoch.
  EXPECT_EQ(result->values.back().source_timestamp,
            to - std::chrono::minutes(30));
  EXPECT_EQ(result->values.front().source_timestamp,
            to - std::chrono::minutes(30) * 48);
}

// Regression: a raw read used to fall back to a synthesized series around a
// mean of 100.0 for any node with no registered profile. That handed consumers
// a convincing trend for a node the fixture never described — a table row
// bound to a node its address space does not have painted a sparkline right
// next to its "no data" quality mark.
TEST(LocalHistoryService, UnknownNodeHasNoHistory) {
  TestExecutor executor;
  LocalHistoryService service;
  service.SetRawProfile(NodeId{7, 2}, 42.0);

  auto result =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = NodeId{9, 2},
                                  .from = scada::Now() - std::chrono::hours(1),
                                  .to = scada::Now()}));

  ASSERT_TRUE(result.ok()) << result.status();
  EXPECT_THAT(result->values, testing::IsEmpty());
}

// Regression: the fixture's `nodes` array may name a node the caller could not
// create (the screenshot fixture's `tree` gives it no parent). Seeding a raw
// profile for it would let a consumer read history for a node that does not
// exist, so LoadFromJson skips whatever the caller's predicate rejects.
TEST(LocalHistoryService, LoadFromJsonSkipsNodesTheCallerDoesNotHave) {
  TestExecutor executor;
  LocalHistoryService service;

  const NodeId present = NodeIdFromScadaString("TIT.200");
  const NodeId orphan = NodeIdFromScadaString("TIT.209");
  ASSERT_FALSE(present.is_null());
  ASSERT_NE(present, orphan);

  service.LoadFromJson(
      boost::json::parse(R"({
        "now": "2026-04-16 15:02:00",
        "nodes": [
          {"id": "TIT.200", "base_value": 42.0},
          {"id": "TIT.209", "base_value": 0.95}
        ],
        "events": []
      })"),
      [&present](const NodeId& node_id) { return node_id == present; });

  const auto to = scada::Now();
  const auto from = to - std::chrono::hours(1);
  auto kept =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = present, .from = from, .to = to}));
  ASSERT_TRUE(kept.ok()) << kept.status();
  EXPECT_EQ(kept->values.size(), 48u);

  auto skipped =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = orphan, .from = from, .to = to}));
  ASSERT_TRUE(skipped.ok()) << skipped.status();
  EXPECT_THAT(skipped->values, testing::IsEmpty());
}

// LoadFromJson leaves an event marked `"acknowledged": false` pending (null
// acknowledged time), so alarm-surface fixtures can render actionable state;
// unmarked events stay acknowledged as before.
TEST(LocalHistoryService, LoadFromJsonHonorsAcknowledgedFlag) {
  TestExecutor executor;
  LocalHistoryService service;
  service.LoadFromJson(boost::json::parse(R"({
    "now": "2026-04-16 15:02:00",
    "nodes": [],
    "events": [
      {"id": 1, "hours_ago": 1.0, "severity": "critical", "message": "m1",
       "node_id": "TS.105", "change_mask": 16, "acknowledged": false},
      {"id": 2, "hours_ago": 2.0, "severity": "normal", "message": "m2",
       "node_id": "TS.105", "change_mask": 16}
    ]
  })"));

  auto result = WaitAwaitable(
      executor, service.HistoryReadEvents(NodeId{}, scada::Time{}, scada::Now(),
                                          EventFilter{}));

  ASSERT_TRUE(result.ok()) << result.status();
  ASSERT_EQ(result->events.size(), 2u);
  EXPECT_FALSE(result->events[0].acked);
  EXPECT_TRUE(scada::IsNull(result->events[0].acknowledged_time));
  EXPECT_TRUE(result->events[1].acked);
  EXPECT_FALSE(scada::IsNull(result->events[1].acknowledged_time));
}

TEST(LocalHistoryService, CoroutineHistoryReadEventsReturnsStoredEvents) {
  TestExecutor executor;
  LocalHistoryService service;
  Event event;
  event.event_id = EventId{17};
  event.source_node_id = NodeId{3, 2};
  event.time = scada::Now();
  event.receive_time = event.time;
  event.severity = kSeverityWarning;
  event.message = LocalizedText{u"Warning"};
  service.AddEvent(event);

  auto result = WaitAwaitable(
      executor, service.HistoryReadEvents(
                    event.source_node_id, event.time - std::chrono::hours(1),
                    event.time + std::chrono::hours(1), EventFilter{}));

  ASSERT_TRUE(result.ok()) << result.status();
  ASSERT_EQ(result->events.size(), 1u);
  EXPECT_EQ(result->events[0].event_id, event.event_id);
  EXPECT_EQ(result->events[0].source_node_id, event.source_node_id);
  EXPECT_EQ(result->events[0].severity, kSeverityWarning);
}

}  // namespace
}  // namespace scada
