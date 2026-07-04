#include "address_space/address_space_impl.h"
#include "address_space/local_history_service.h"
#include "address_space/local_method_service.h"
#include "address_space/local_node_management_service.h"
#include "address_space/local_session_service.h"
#include "address_space/method_service_impl.h"

#include "base/test/awaitable_test.h"
#include "scada/standard_node_ids.h"
#include "scada/test/status_matchers.h"

#include <gmock/gmock.h>

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

  const auto status =
      WaitAwaitable(executor, service.Call(NodeId{1, 2}, NodeId{2, 2}, {},
                                           ServiceContext{}));

  EXPECT_EQ(status.code(), StatusCode::Bad_WrongMethodId);
}

TEST(LocalNodeManagementService, CoroutineAddNodesReturnsBadResults) {
  TestExecutor executor;
  LocalNodeManagementService service;

  auto result = WaitAwaitable(
      executor, service.AddNodes({AddNodesItem{
                    .requested_id = NodeId{1, 2},
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
      service.DeleteNodes({DeleteNodesItem{.node_id = NodeId{1, 2}}}));

  EXPECT_THAT(result, test::StatusIs(StatusCode::Bad));
}

TEST(LocalNodeManagementService, CoroutineAddReferencesReturnsBadResults) {
  TestExecutor executor;
  LocalNodeManagementService service;

  auto result =
      WaitAwaitable(executor,
                    service.AddReferences({AddReferencesItem{
                        .source_node_id = id::ObjectsFolder,
                        .reference_type_id = id::Organizes,
                        .target_node_id = NodeId{1, 2}}}));

  EXPECT_THAT(result, test::StatusIs(StatusCode::Bad));
}

TEST(LocalNodeManagementService, CoroutineDeleteReferencesReturnsBadResults) {
  TestExecutor executor;
  LocalNodeManagementService service;

  auto result =
      WaitAwaitable(executor,
                    service.DeleteReferences({DeleteReferencesItem{
                        .source_node_id = id::ObjectsFolder,
                        .reference_type_id = id::Organizes,
                        .target_node_id = NodeId{1, 2}}}));

  EXPECT_THAT(result, test::StatusIs(StatusCode::Bad));
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

TEST(LocalHistoryService, CoroutineHistoryReadEventsReturnsStoredEvents) {
  TestExecutor executor;
  LocalHistoryService service;
  Event event;
  event.event_id = EventId{17};
  event.node_id = NodeId{3, 2};
  event.time = base::Time::Now();
  event.receive_time = event.time;
  event.severity = kSeverityWarning;
  event.message = LocalizedText{u"Warning"};
  service.AddEvent(event);

  auto result = WaitAwaitable(
      executor, service.HistoryReadEvents(event.node_id,
                                          event.time -
                                              base::TimeDelta::FromHours(1),
                                          event.time +
                                              base::TimeDelta::FromHours(1),
                                          EventFilter{}));

  EXPECT_TRUE(result.status);
  ASSERT_EQ(result.events.size(), 1u);
  EXPECT_EQ(result.events[0].event_id, event.event_id);
  EXPECT_EQ(result.events[0].node_id, event.node_id);
  EXPECT_EQ(result.events[0].severity, kSeverityWarning);
}

}  // namespace
}  // namespace scada
