#include "vidicon/services/vidicon_session.h"

#include "base/test/awaitable_test.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

namespace {

using testing::Not;

TEST(VidiconSession, CoroutineSessionServiceReportsLocalSessionMetadata) {
  VidiconSession session;
  auto& coroutine_session = session.coroutine_session_service();

  base::TimeDelta ping_delay;
  EXPECT_TRUE(coroutine_session.IsConnected(&ping_delay));
  EXPECT_TRUE(ping_delay.is_zero());
  EXPECT_TRUE(coroutine_session.HasPrivilege(scada::Privilege::Configure));
  EXPECT_FALSE(coroutine_session.IsScada());
  EXPECT_EQ(coroutine_session.GetUserId(), scada::NodeId{});
  EXPECT_EQ(coroutine_session.GetHostName(), "Vidicon");
  EXPECT_EQ(coroutine_session.GetSessionDebugger(), nullptr);
}

TEST(VidiconSession, CoroutineSessionServiceLifecycleCompletes) {
  VidiconSession session;
  const auto executor = std::make_shared<TestExecutor>();
  auto& coroutine_session = session.coroutine_session_service();

  EXPECT_NO_THROW(WaitAwaitable(executor, coroutine_session.Reconnect()));
  EXPECT_NO_THROW(WaitAwaitable(executor, coroutine_session.Disconnect()));
}

TEST(VidiconSession, CoroutineReadUsesLocalAddressSpace) {
  VidiconSession session;
  const auto executor = std::make_shared<TestExecutor>();
  auto inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      std::vector<scada::ReadValueId>{
          {.node_id = scada::id::RootFolder,
           .attribute_id = scada::AttributeId::DisplayName}});

  auto [status, results] = WaitAwaitable(
      executor, static_cast<scada::CoroutineAttributeService&>(session).Read(
                    scada::ServiceContext{}, inputs));

  EXPECT_TRUE(status);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].status_code, scada::StatusCode::Good);
  EXPECT_FALSE(results[0].value.is_null());
}

TEST(VidiconSession, CoroutineBrowseUsesLocalAddressSpace) {
  VidiconSession session;
  const auto executor = std::make_shared<TestExecutor>();

  auto [status, results] = WaitAwaitable(
      executor, static_cast<scada::CoroutineViewService&>(session).Browse(
                    scada::ServiceContext{},
                    {{scada::id::RootFolder,
                      scada::BrowseDirection::Forward,
                      scada::id::HierarchicalReferences,
                      true}}));

  EXPECT_TRUE(status);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].status_code, scada::StatusCode::Good);
  EXPECT_THAT(results[0].references, Not(testing::IsEmpty()));
}

TEST(VidiconSession, CoroutineHistoryReturnsBad) {
  VidiconSession session;
  const auto executor = std::make_shared<TestExecutor>();

  const auto raw_result = WaitAwaitable(
      executor,
      static_cast<scada::CoroutineHistoryService&>(session).HistoryReadRaw(
          scada::HistoryReadRawDetails{.node_id = scada::id::RootFolder}));
  const auto events_result = WaitAwaitable(
      executor,
      static_cast<scada::CoroutineHistoryService&>(session).HistoryReadEvents(
          scada::id::Server, {}, {}, {}));

  EXPECT_EQ(raw_result.status.code(), scada::StatusCode::Bad);
  EXPECT_TRUE(raw_result.values.empty());
  EXPECT_EQ(events_result.status.code(), scada::StatusCode::Bad);
  EXPECT_TRUE(events_result.events.empty());
}

TEST(VidiconSession, CoroutineWriteAndMethodReturnBad) {
  VidiconSession session;
  const auto executor = std::make_shared<TestExecutor>();
  auto inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      std::vector<scada::WriteValue>{{.node_id = scada::id::RootFolder}});

  auto [write_status, write_results] = WaitAwaitable(
      executor, static_cast<scada::CoroutineAttributeService&>(session).Write(
                    scada::ServiceContext{}, inputs));
  const auto call_status = WaitAwaitable(
      executor, static_cast<scada::CoroutineMethodService&>(session).Call(
                    scada::id::RootFolder, scada::NodeId{1, 2}, {},
                    scada::NodeId{}));

  EXPECT_EQ(write_status.code(), scada::StatusCode::Bad);
  EXPECT_TRUE(write_results.empty());
  EXPECT_EQ(call_status.code(), scada::StatusCode::Bad);
}

TEST(VidiconSession, CoroutineNodeManagementReturnsBad) {
  VidiconSession session;
  const auto executor = std::make_shared<TestExecutor>();
  auto& service =
      static_cast<scada::CoroutineNodeManagementService&>(session);

  auto [add_nodes_status, add_nodes_results] = WaitAwaitable(
      executor,
      service.AddNodes({scada::AddNodesItem{
          .requested_id = scada::NodeId{1, 2},
          .parent_id = scada::id::RootFolder,
          .node_class = scada::NodeClass::Object,
          .type_definition_id = scada::id::BaseObjectType}}));
  auto [delete_nodes_status, delete_nodes_results] = WaitAwaitable(
      executor, service.DeleteNodes(
                    {scada::DeleteNodesItem{.node_id = scada::NodeId{1, 2}}}));
  auto [add_references_status, add_references_results] = WaitAwaitable(
      executor,
      service.AddReferences({scada::AddReferencesItem{
          .source_node_id = scada::id::RootFolder,
          .reference_type_id = scada::id::Organizes,
          .target_node_id = scada::NodeId{1, 2}}}));
  auto [delete_references_status, delete_references_results] = WaitAwaitable(
      executor,
      service.DeleteReferences({scada::DeleteReferencesItem{
          .source_node_id = scada::id::RootFolder,
          .reference_type_id = scada::id::Organizes,
          .target_node_id = scada::NodeId{1, 2}}}));

  EXPECT_EQ(add_nodes_status.code(), scada::StatusCode::Bad);
  EXPECT_TRUE(add_nodes_results.empty());
  EXPECT_EQ(delete_nodes_status.code(), scada::StatusCode::Bad);
  EXPECT_TRUE(delete_nodes_results.empty());
  EXPECT_EQ(add_references_status.code(), scada::StatusCode::Bad);
  EXPECT_TRUE(add_references_results.empty());
  EXPECT_EQ(delete_references_status.code(), scada::StatusCode::Bad);
  EXPECT_TRUE(delete_references_results.empty());
}

}  // namespace
