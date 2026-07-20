#include "vidicon/services/vidicon_session.h"

#include "base/test/awaitable_test.h"
#include "scada/standard_node_ids.h"
#include "scada/test/status_matchers.h"

#include <gmock/gmock.h>

namespace {

using testing::Not;

TEST(VidiconSession, SessionServiceReportsLocalSessionMetadata) {
  VidiconSession session;
  auto& coroutine_session = session;

  scada::base::TimeDelta ping_delay;
  EXPECT_TRUE(coroutine_session.IsConnected(&ping_delay));
  EXPECT_TRUE(ping_delay.is_zero());
  EXPECT_TRUE(coroutine_session.HasPrivilege(scada::Privilege::Configure));
  EXPECT_FALSE(coroutine_session.IsScada());
  EXPECT_EQ(coroutine_session.GetUserId(), scada::NodeId{});
  EXPECT_EQ(coroutine_session.GetHostName(), "Vidicon");
  EXPECT_EQ(coroutine_session.GetSessionDebugger(), nullptr);
}

TEST(VidiconSession, SessionServiceLifecycleCompletes) {
  VidiconSession session;
  TestExecutor executor;
  auto& coroutine_session = session;

  EXPECT_NO_THROW(WaitAwaitable(executor, coroutine_session.Reconnect()));
  EXPECT_NO_THROW(WaitAwaitable(executor, coroutine_session.Disconnect()));
}

TEST(VidiconSession, CoroutineReadUsesLocalAddressSpace) {
  VidiconSession session;
  TestExecutor executor;
  std::vector<scada::ReadValueId> inputs{
      {.node_id = scada::id::RootFolder,
       .attribute_id = scada::AttributeId::DisplayName}};

  ASSERT_OK_AND_ASSIGN(auto results, WaitAwaitable(
      executor, static_cast<scada::AttributeService&>(session).Read(
                    scada::ServiceContext{}, inputs)));

  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].status_code, scada::StatusCode::Good);
  EXPECT_FALSE(results[0].value.is_null());
}

TEST(VidiconSession, CoroutineBrowseUsesLocalAddressSpace) {
  VidiconSession session;
  TestExecutor executor;

  ASSERT_OK_AND_ASSIGN(auto results, WaitAwaitable(
      executor, static_cast<scada::ViewService&>(session).Browse(
                    scada::ServiceContext{},
                    {{scada::id::RootFolder,
                      scada::BrowseDirection::Forward,
                      scada::id::HierarchicalReferences,
                      true}})));

  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].status_code, scada::StatusCode::Good);
  EXPECT_THAT(results[0].references, Not(testing::IsEmpty()));
}

TEST(VidiconSession, CoroutineHistoryReturnsBad) {
  VidiconSession session;
  TestExecutor executor;

  const auto raw_result = WaitAwaitable(
      executor,
      static_cast<scada::HistoryService&>(session).HistoryReadRaw(
          scada::HistoryReadRawDetails{.node_id = scada::id::RootFolder}));
  const auto events_result = WaitAwaitable(
      executor,
      static_cast<scada::HistoryService&>(session).HistoryReadEvents(
          scada::id::Server, {}, {}, {}));

  EXPECT_EQ(raw_result.status.code(), scada::StatusCode::Bad);
  EXPECT_TRUE(raw_result.values.empty());
  EXPECT_EQ(events_result.status.code(), scada::StatusCode::Bad);
  EXPECT_TRUE(events_result.events.empty());
}

TEST(VidiconSession, CoroutineWriteAndMethodReturnBad) {
  VidiconSession session;
  TestExecutor executor;
  std::vector<scada::WriteValue> inputs{{.node_id = scada::id::RootFolder}};

  const auto write_result = WaitAwaitable(
      executor, static_cast<scada::AttributeService&>(session).Write(
                    scada::ServiceContext{}, inputs));
  const auto call_status = WaitAwaitable(
      executor, static_cast<scada::MethodService&>(session).Call(
                    scada::id::RootFolder, scada::NodeId{1, 2}, {},
                    scada::NodeId{}));

  EXPECT_THAT(write_result, scada::test::StatusIs(scada::StatusCode::Bad));
  EXPECT_EQ(call_status.code(), scada::StatusCode::Bad);
}

TEST(VidiconSession, CoroutineNodeManagementReturnsBad) {
  VidiconSession session;
  TestExecutor executor;
  auto& service =
      static_cast<scada::NodeManagementService&>(session);

  const auto add_nodes_result = WaitAwaitable(
      executor,
      service.AddNodes({scada::AddNodesItem{
          .requested_id = scada::NodeId{1, 2},
          .parent_id = scada::id::RootFolder,
          .node_class = scada::NodeClass::Object,
          .type_definition_id = scada::id::BaseObjectType}}));
  const auto delete_nodes_result = WaitAwaitable(
      executor, service.DeleteNodes(
                    {scada::DeleteNodesItem{.node_id = scada::NodeId{1, 2}}}));
  const auto add_references_result = WaitAwaitable(
      executor,
      service.AddReferences({scada::AddReferencesItem{
          .source_node_id = scada::id::RootFolder,
          .reference_type_id = scada::id::Organizes,
          .target_node_id = scada::NodeId{1, 2}}}));
  const auto delete_references_result = WaitAwaitable(
      executor,
      service.DeleteReferences({scada::DeleteReferencesItem{
          .source_node_id = scada::id::RootFolder,
          .reference_type_id = scada::id::Organizes,
          .target_node_id = scada::NodeId{1, 2}}}));

  EXPECT_THAT(add_nodes_result, scada::test::StatusIs(scada::StatusCode::Bad));
  EXPECT_THAT(delete_nodes_result,
              scada::test::StatusIs(scada::StatusCode::Bad));
  EXPECT_THAT(add_references_result,
              scada::test::StatusIs(scada::StatusCode::Bad));
  EXPECT_THAT(delete_references_result,
              scada::test::StatusIs(scada::StatusCode::Bad));
}

}  // namespace
