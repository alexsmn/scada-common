#include "address_space/address_space_impl.h"
#include "address_space/local_history_service.h"
#include "address_space/local_method_service.h"
#include "address_space/local_monitored_item_service.h"
#include "address_space/local_node_management_service.h"
#include "address_space/local_session_service.h"
#include "address_space/method_service_impl.h"
#include "address_space/test/test_address_space.h"

#include "base/test/awaitable_test.h"
#include "scada/monitored_item.h"
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
  EXPECT_TRUE(service.HasPrivilege(Privilege::Configure));
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

  EXPECT_EQ(status.code(), StatusCode::Bad);
}

TEST(MethodServiceImpl, CoroutineCallReturnsWrongMethodId) {
  TestExecutor executor;
  AddressSpaceImpl address_space;
  MethodServiceImpl service{{address_space}};

  const auto status = WaitAwaitable(
      executor, service.Call(NodeId{1, 2}, NodeId{2, 2}, {}, ServiceContext{}));

  EXPECT_EQ(status.code(), StatusCode::Bad_WrongMethodId);
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
  EXPECT_FALSE(data_change->value.source_timestamp.is_null());
  EXPECT_FALSE(data_change->value.server_timestamp.is_null());
}

TEST(LocalHistoryService, CoroutineHistoryReadRawReturnsGeneratedProfile) {
  TestExecutor executor;
  LocalHistoryService service;
  const NodeId node_id{7, 2};
  service.SetRawProfile(node_id, 42.0);

  auto result = WaitAwaitable(
      executor, service.HistoryReadRaw(HistoryReadRawDetails{
                    .node_id = node_id,
                    .from = base::Time::Now() - base::TimeDelta::FromHours(1),
                    .to = base::Time::Now()}));

  EXPECT_TRUE(result.status);
  ASSERT_EQ(result.values.size(), 48u);
  EXPECT_EQ(result.values.front().status_code, StatusCode::Good);
  EXPECT_FALSE(result.values.front().value.is_null());
}

// The synthesized series spans the requested window, so a narrow range (a
// table row's 1 h sparkline window) still reads a fully-populated series
// instead of the tail of a fixed 30-minute-spaced day.
TEST(LocalHistoryService, GeneratedProfileSpansTheRequestedRange) {
  TestExecutor executor;
  LocalHistoryService service;
  const NodeId node_id{7, 2};
  service.SetRawProfile(node_id, 42.0);

  const auto to = base::Time::Now();
  const auto from = to - base::TimeDelta::FromHours(1);
  auto result =
      WaitAwaitable(executor, service.HistoryReadRaw(HistoryReadRawDetails{
                                  .node_id = node_id, .from = from, .to = to}));

  EXPECT_TRUE(result.status);
  ASSERT_EQ(result.values.size(), 48u);
  // All 48 points fall inside [from, to], evenly spaced, ending at `to`.
  EXPECT_GE(result.values.front().source_timestamp, from);
  EXPECT_EQ(result.values.back().source_timestamp, to - (to - from) / 48);
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
      executor, service.HistoryReadEvents(NodeId{}, base::Time{},
                                          base::Time::Now(), EventFilter{}));

  ASSERT_EQ(result.events.size(), 2u);
  EXPECT_FALSE(result.events[0].acked);
  EXPECT_TRUE(result.events[0].acknowledged_time.is_null());
  EXPECT_TRUE(result.events[1].acked);
  EXPECT_FALSE(result.events[1].acknowledged_time.is_null());
}

TEST(LocalHistoryService, CoroutineHistoryReadEventsReturnsStoredEvents) {
  TestExecutor executor;
  LocalHistoryService service;
  Event event;
  event.event_id = EventId{17};
  event.source_node_id = NodeId{3, 2};
  event.time = base::Time::Now();
  event.receive_time = event.time;
  event.severity = kSeverityWarning;
  event.message = LocalizedText{u"Warning"};
  service.AddEvent(event);

  auto result = WaitAwaitable(
      executor, service.HistoryReadEvents(
                    event.source_node_id, event.time - base::TimeDelta::FromHours(1),
                    event.time + base::TimeDelta::FromHours(1), EventFilter{}));

  EXPECT_TRUE(result.status);
  ASSERT_EQ(result.events.size(), 1u);
  EXPECT_EQ(result.events[0].event_id, event.event_id);
  EXPECT_EQ(result.events[0].node_id, event.source_node_id);
  EXPECT_EQ(result.events[0].severity, kSeverityWarning);
}

}  // namespace
}  // namespace scada
