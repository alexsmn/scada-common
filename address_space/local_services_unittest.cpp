#include "address_space/address_space_impl.h"
#include "address_space/local_history_service.h"
#include "address_space/local_method_service.h"
#include "address_space/local_monitored_item_service.h"
#include "address_space/local_node_management_service.h"
#include "address_space/local_session_service.h"
#include "address_space/method_service_impl.h"
#include "address_space/test/test_address_space.h"

#include "base/test/awaitable_test.h"
#include "base/time/calendar.h"
#include "common/sync_attribute_service.h"
#include "scada/date_time.h"
#include "scada/monitored_item.h"
#include "scada/read_value_id.h"
#include "scada/standard_node_ids.h"
#include "scada/test/status_matchers.h"

#include <boost/json.hpp>
#include <gmock/gmock.h>
#include <map>
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

// A `SessionService` that returns `true` must write `ping_delay`: the type is
// a `std::chrono` duration, so a caller's default-initialized local holds an
// uninitialized rep and reads back garbage. That is exactly what made the
// screenshot generator's status strip and event journal differ between two
// runs of one binary on one fixture.
TEST(LocalSessionService, ConnectedSessionReportsAZeroPingDelay) {
  LocalSessionService service;

  Duration ping_delay = std::chrono::hours{1};
  EXPECT_TRUE(service.IsConnected(&ping_delay));
  EXPECT_EQ(ping_delay, Duration::zero());
}

TEST(LocalSessionService, ReportsANullUserIdUntilOneIsSet) {
  LocalSessionService service;

  EXPECT_TRUE(service.GetUserId().is_null());
}

TEST(LocalSessionService, ReportsTheUserIdItWasGiven) {
  LocalSessionService service;

  const NodeId user_id{42u, 1};
  service.SetUserId(user_id);

  EXPECT_EQ(service.GetUserId(), user_id);
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
  ASSERT_FALSE(result->values.empty());
  EXPECT_EQ(result->values.front().status_code, StatusCode::Good);
  EXPECT_FALSE(result->values.front().value.is_null());
}

// A narrow window (a table row's 1 h sparkline) reads every sample of the
// stored series inside it, on the series' fixed spacing.
TEST(LocalHistoryService, ReadReturnsTheSeriesSamplesInsideTheWindow) {
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
  ASSERT_GE(result->values.size(), 11u);
  EXPECT_LE(result->values.size(), 12u);
  EXPECT_GE(result->values.front().source_timestamp, from);
  EXPECT_LT(result->values.back().source_timestamp, to);
  for (size_t i = 1; i < result->values.size(); ++i) {
    EXPECT_EQ(result->values[i].source_timestamp -
                  result->values[i - 1].source_timestamp,
              std::chrono::minutes(5));
  }
}

// A read whose upper bound is the "current-only" sentinel (Time::Max, used
// by live consumers that want the latest sample rather than a finite window)
// must end at now, not at Max. The series used to be anchored to the read's
// upper bound, and anchoring it to Max spread its points across ~285,000
// years, so every point but the first fell outside any real query window and
// the series read flat (the trend and its value grid showed constant lines
// with Min == Max == Average).
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
  ASSERT_FALSE(result->values.empty());
  const auto now = scada::Now();
  EXPECT_LT(result->values.back().source_timestamp, now);
  EXPECT_GT(result->values.back().source_timestamp,
            now - std::chrono::minutes(10));
}

// A window far wider than the data is meant to cover must not stretch the
// series to fit it. The trend probes for its earliest available sample to fill
// the left edge of the plot, which asks for everything from the epoch to now;
// spreading a fixed number of points across that put them ~14 months apart, so
// every point but the last fell outside the 24 h the graph was displaying. The
// plot came out empty while the axes, limit bands and legend were all correct,
// which read as a rendering quirk rather than as bad data.
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
  // A day at five-minute spacing, ending just before `to`.
  EXPECT_GE(result->values.size(), 287u);
  EXPECT_LE(result->values.size(), 288u);
  EXPECT_GE(result->values.front().source_timestamp,
            to - std::chrono::hours(24));
  EXPECT_LT(result->values.back().source_timestamp, to);
}

// Regression for backlog 949. The series used to be synthesized to fit each
// read's window, so a 1 h read and a 16 h read of the same node gave different
// values at the same instants — and the client's timed-data cache merges both,
// so a table sparkline depended on which other capture had read the node
// first. Every read must be a slice of one series.
TEST(LocalHistoryService, OverlappingReadsAgreeAtEveryInstant) {
  TestExecutor executor;
  LocalHistoryService service;
  const NodeId node_id{211, 2};
  service.SetRawProfile(node_id, 42.3);
  const auto now = scada::Time{} + std::chrono::days(20559) +
                   std::chrono::hours(15) + std::chrono::minutes(2);
  service.SetNowOverride(now);

  auto narrow =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = node_id,
                                  .from = now - std::chrono::hours(1),
                                  .to = scada::kMaxTime}));
  auto wide =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = node_id,
                                  .from = now - std::chrono::hours(15),
                                  .to = now + std::chrono::hours(1)}));
  ASSERT_TRUE(narrow.ok()) << narrow.status();
  ASSERT_TRUE(wide.ok()) << wide.status();
  ASSERT_FALSE(narrow->values.empty());

  std::map<scada::Time, double> by_time;
  for (const auto& v : wide->values)
    by_time[v.source_timestamp] = v.value.as_double();
  for (const auto& v : narrow->values) {
    auto it = by_time.find(v.source_timestamp);
    ASSERT_NE(it, by_time.end())
        << "the wide read lacks an instant the narrow read has";
    EXPECT_EQ(it->second, v.value.as_double());
  }
}

// The window-anchored series also ran to the read's upper bound when that was
// after now, so a graph showing a little past the present was handed samples
// from the future. A historian holds nothing past now.
TEST(LocalHistoryService, ReadEndingInTheFutureStopsAtNow) {
  TestExecutor executor;
  LocalHistoryService service;
  const NodeId node_id{10, 2};
  service.SetRawProfile(node_id, 42.0);
  const auto now = scada::Now();
  service.SetNowOverride(now);

  auto result =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = node_id,
                                  .from = now - std::chrono::hours(16),
                                  .to = now + std::chrono::hours(1)}));

  ASSERT_TRUE(result.ok()) << result.status();
  ASSERT_FALSE(result->values.empty());
  EXPECT_LT(result->values.back().source_timestamp, now);
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

  // Read relative to the fixture's frozen clock: the series ends there.
  const auto to = *base::TimeFromString("2026-04-16 15:02:00",
                                        /*is_local=*/true);
  const auto from = to - std::chrono::hours(1);
  auto kept =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = present, .from = from, .to = to}));
  ASSERT_TRUE(kept.ok()) << kept.status();
  EXPECT_FALSE(kept->values.empty());

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

// A fixture may write `severity` as a raw OPC UA number instead of one of the
// three band literals, which only reach three of the thousand values. Numbers
// outside 1..1000 are clamped rather than rejected, so a typo cannot hand the
// severity banding a value the scale has no meaning for.
TEST(LocalHistoryService, LoadFromJsonAcceptsANumericSeverity) {
  TestExecutor executor;
  LocalHistoryService service;
  service.LoadFromJson(boost::json::parse(R"({
    "now": "2026-04-16 15:02:00",
    "nodes": [],
    "events": [
      {"id": 1, "hours_ago": 1.0, "severity": 450, "message": "m1",
       "node_id": "TS.105", "change_mask": 16},
      {"id": 2, "hours_ago": 2.0, "severity": "warning", "message": "m2",
       "node_id": "TS.105", "change_mask": 16},
      {"id": 3, "hours_ago": 3.0, "severity": 9000, "message": "m3",
       "node_id": "TS.105", "change_mask": 16},
      {"id": 4, "hours_ago": 4.0, "severity": 0, "message": "m4",
       "node_id": "TS.105", "change_mask": 16}
    ]
  })"));

  auto result = WaitAwaitable(
      executor, service.HistoryReadEvents(NodeId{}, scada::Time{}, scada::Now(),
                                          EventFilter{}));

  ASSERT_TRUE(result.ok()) << result.status();
  ASSERT_EQ(result->events.size(), 4u);
  EXPECT_EQ(result->events[0].severity, 450u);
  // The band literals keep working alongside the numeric form.
  EXPECT_EQ(result->events[1].severity, kSeverityWarning);
  EXPECT_EQ(result->events[2].severity, 1000u);
  EXPECT_EQ(result->events[3].severity, 1u);
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

// Event subscriptions are served from seeded events, not the address space:
// the device log consumes events, and without this it renders an empty grid no
// matter what a fixture says.
TEST(LocalMonitoredItemService, DeliversSeededEventsToAnEventSubscription) {
  TestExecutor executor;
  ::TestAddressSpace address_space;
  LocalMonitoredItemService service{address_space.sync_attribute_service_impl};

  Event first;
  first.message = LocalizedText{u"first"};
  Event second;
  second.message = LocalizedText{u"second"};
  service.AddEvent(address_space.kTestNode1Id, std::any{first});
  service.AddEvent(address_space.kTestNode1Id, std::any{second});

  ASSERT_OK_AND_ASSIGN(auto subscription,
                       service.CreateSubscription(ServiceContext{}, {}));
  auto results = WaitAwaitable(
      executor,
      subscription->AddItems({MonitoredItemCreateRequest{
          .item_to_monitor = {.node_id = address_space.kTestNode1Id,
                              .attribute_id = AttributeId::EventNotifier},
          .parameters = {.filter = EventFilter{}},
          .client_handle = 1}}));
  ASSERT_EQ(results.size(), 1u);
  ASSERT_TRUE(results[0].status);

  // The subscription hands out whatever is queued when a reader arrives, so a
  // two-event delivery can span two reads.
  std::vector<std::u16string> messages;
  while (messages.size() < 2) {
    ASSERT_OK_AND_ASSIGN(auto notifications,
                         WaitAwaitable(executor, subscription->ReadNext(10)));
    ASSERT_FALSE(notifications.empty());
    for (const auto& notification : notifications) {
      const auto* event = std::get_if<EventNotification>(&notification);
      ASSERT_TRUE(event) << "an event subscription must not yield data changes";
      if (event->event.has_value())
        messages.push_back(std::any_cast<Event>(event->event).message.text);
    }
  }

  EXPECT_EQ(messages, (std::vector<std::u16string>{u"first", u"second"}));
}

// Scoped by source node: two views seeded from one fixture must stay
// distinguishable, and a device log showing another device's traffic would be
// a convincing lie.
TEST(LocalMonitoredItemService, DeliversOnlyTheSubscribedNodesEvents) {
  TestExecutor executor;
  ::TestAddressSpace address_space;
  LocalMonitoredItemService service{address_space.sync_attribute_service_impl};

  Event mine;
  mine.message = LocalizedText{u"mine"};
  Event theirs;
  theirs.message = LocalizedText{u"theirs"};
  service.AddEvent(address_space.kTestNode1Id, std::any{mine});
  service.AddEvent(address_space.kTestNode2Id, std::any{theirs});

  ASSERT_OK_AND_ASSIGN(auto subscription,
                       service.CreateSubscription(ServiceContext{}, {}));
  auto results = WaitAwaitable(
      executor,
      subscription->AddItems({MonitoredItemCreateRequest{
          .item_to_monitor = {.node_id = address_space.kTestNode1Id,
                              .attribute_id = AttributeId::EventNotifier},
          .parameters = {.filter = EventFilter{}},
          .client_handle = 1}}));
  ASSERT_EQ(results.size(), 1u);

  ASSERT_OK_AND_ASSIGN(auto notifications,
                       WaitAwaitable(executor, subscription->ReadNext(10)));
  ASSERT_EQ(notifications.size(), 1u);
  const auto* only = std::get_if<EventNotification>(&notifications[0]);
  ASSERT_TRUE(only);
  ASSERT_TRUE(only->event.has_value());
  EXPECT_EQ(std::any_cast<Event>(only->event).message.text, u"mine");
}

// The journal reads rooted at the Server object and the device log reads its
// own device from the same seeded set; without node scoping the device log
// lists every other node's events too.
TEST(LocalHistoryService, ScopesEventReadsToTheRequestedNode) {
  TestExecutor executor;
  LocalHistoryService service;

  const NodeId device{7, 1};
  const NodeId other{8, 1};
  Event mine;
  mine.source_node_id = device;
  mine.message = LocalizedText{u"mine"};
  Event theirs;
  theirs.source_node_id = other;
  theirs.message = LocalizedText{u"theirs"};
  service.AddEvent(mine);
  service.AddEvent(theirs);

  ASSERT_OK_AND_ASSIGN(
      auto device_events,
      WaitAwaitable(executor, service.HistoryReadEvents(device, kNullTime,
                                                        kNullTime, {})));
  ASSERT_EQ(device_events.events.size(), 1u);
  EXPECT_EQ(device_events.events[0].message.text, u"mine");

  // The whole-server journal still sees everything.
  ASSERT_OK_AND_ASSIGN(
      auto all_events,
      WaitAwaitable(executor, service.HistoryReadEvents(id::Server, kNullTime,
                                                        kNullTime, {})));
  EXPECT_EQ(all_events.events.size(), 2u);
}

}  // namespace scada
