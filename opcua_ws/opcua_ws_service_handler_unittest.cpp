#include "opcua_ws/opcua_ws_service_handler.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/node_management_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>

using namespace testing;

namespace opcua_ws {
namespace {

class OpcUaWsServiceHandlerTest : public Test {
 protected:
  static scada::NodeId NumericNode(scada::NumericId id,
                                   scada::NamespaceIndex ns = 2) {
    return {id, ns};
  }

  StrictMock<scada::MockHistoryService> history_service_;
  StrictMock<scada::MockMethodService> method_service_;
  StrictMock<scada::MockNodeManagementService> node_management_service_;
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  const scada::NodeId user_id_ = NumericNode(700, 3);
  OpcUaWsServiceHandler handler_{
      {executor_, history_service_, method_service_, node_management_service_,
       user_id_}};
};

TEST_F(OpcUaWsServiceHandlerTest,
       HandleCall_ForwardsEachMethodWithSessionUserId) {
  CallRequest request{.methods = {
                          {.object_id = NumericNode(10),
                           .method_id = NumericNode(11),
                           .arguments = {scada::Variant{true}}},
                          {.object_id = NumericNode(12),
                           .method_id = NumericNode(13),
                           .arguments = {scada::Variant{scada::Int32{42}}}},
                      }};

  Sequence seq;
  EXPECT_CALL(method_service_,
              Call(request.methods[0].object_id,
                   request.methods[0].method_id,
                   request.methods[0].arguments,
                   user_id_,
                   _))
      .WillOnce(Invoke([](const scada::NodeId&,
                          const scada::NodeId&,
                          const std::vector<scada::Variant>&,
                          const scada::NodeId&,
                          const scada::StatusCallback& callback) {
        callback(scada::StatusCode::Good);
      }));
  EXPECT_CALL(method_service_,
              Call(request.methods[1].object_id,
                   request.methods[1].method_id,
                   request.methods[1].arguments,
                   user_id_,
                   _))
      .WillOnce(Invoke([](const scada::NodeId&,
                          const scada::NodeId&,
                          const std::vector<scada::Variant>&,
                          const scada::NodeId&,
                          const scada::StatusCallback& callback) {
        callback(scada::StatusCode::Bad_WrongCallArguments);
      }));

  auto response = WaitAwaitable(executor_, handler_.Handle(std::move(request)));
  const auto* call_response = std::get_if<CallResponse>(&response);
  ASSERT_NE(call_response, nullptr);
  ASSERT_EQ(call_response->results.size(), 2u);
  EXPECT_EQ(call_response->results[0].status.code(), scada::StatusCode::Good);
  EXPECT_EQ(call_response->results[1].status.code(),
            scada::StatusCode::Bad_WrongCallArguments);
}

TEST_F(OpcUaWsServiceHandlerTest, HandleHistoryReadRaw_PreservesResultPayload) {
  const auto node_id = NumericNode(30);
  const auto from = base::Time::Now() - base::TimeDelta::FromHours(1);
  const auto to = base::Time::Now();
  HistoryReadRawRequest request{
      .details = {.node_id = node_id, .from = from, .to = to, .max_count = 25}};

  EXPECT_CALL(history_service_, HistoryReadRaw(_, _))
      .WillOnce(Invoke([&](const scada::HistoryReadRawDetails& details,
                           const scada::HistoryReadRawCallback& callback) {
        EXPECT_EQ(details.node_id, node_id);
        EXPECT_EQ(details.from, from);
        EXPECT_EQ(details.to, to);
        EXPECT_EQ(details.max_count, 25u);
        callback(scada::HistoryReadRawResult{
            .status = scada::StatusCode::Good,
            .values = {scada::DataValue{scada::Variant{12.5}, {}, to, to}},
            .continuation_point = {1, 2, 3},
        });
      }));

  auto response = WaitAwaitable(executor_, handler_.Handle(std::move(request)));
  const auto* raw_response = std::get_if<HistoryReadRawResponse>(&response);
  ASSERT_NE(raw_response, nullptr);
  EXPECT_EQ(raw_response->result.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(raw_response->result.values.size(), 1u);
  EXPECT_EQ(raw_response->result.values[0].value, scada::Variant{12.5});
  EXPECT_EQ(raw_response->result.continuation_point,
            (scada::ByteString{1, 2, 3}));
}

TEST_F(OpcUaWsServiceHandlerTest,
       HandleHistoryReadEvents_ForwardsFilterAndEvents) {
  scada::HistoryReadEventsDetails details{
      .node_id = NumericNode(40),
      .from = base::Time::Now() - base::TimeDelta::FromHours(4),
      .to = base::Time::Now(),
      .filter = {},
  };

  EXPECT_CALL(history_service_, HistoryReadEvents(_, _, _, _, _))
      .WillOnce(Invoke([&](const scada::NodeId& node_id,
                           base::Time from,
                           base::Time to,
                           const scada::EventFilter&,
                           const scada::HistoryReadEventsCallback& callback) {
        EXPECT_EQ(node_id, details.node_id);
        EXPECT_EQ(from, details.from);
        EXPECT_EQ(to, details.to);
        scada::Event event;
        event.event_id = 99;
        event.time = base::Time::Now();
        event.receive_time = event.time;
        event.node_id = NumericNode(41);
        event.message = scada::LocalizedText{u"alarm"};
        callback(scada::HistoryReadEventsResult{
            .status = scada::StatusCode::Good,
            .events = {std::move(event)},
        });
      }));

  auto response = WaitAwaitable(
      executor_, handler_.Handle(HistoryReadEventsRequest{details}));
  const auto* events_response =
      std::get_if<HistoryReadEventsResponse>(&response);
  ASSERT_NE(events_response, nullptr);
  EXPECT_EQ(events_response->result.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(events_response->result.events.size(), 1u);
  EXPECT_EQ(events_response->result.events[0].event_id, 99u);
}

TEST_F(OpcUaWsServiceHandlerTest, HandleAddNodes_ForwardsBatchResults) {
  AddNodesRequest request{.items = {
                              {.requested_id = NumericNode(50),
                               .parent_id = NumericNode(51),
                               .type_definition_id = NumericNode(52)},
                          }};

  EXPECT_CALL(node_management_service_, AddNodes(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::AddNodesItem>& items,
                           const scada::AddNodesCallback& callback) {
        ASSERT_EQ(items.size(), 1u);
        EXPECT_EQ(items[0].requested_id, NumericNode(50));
        EXPECT_EQ(items[0].parent_id, NumericNode(51));
        EXPECT_EQ(items[0].type_definition_id, NumericNode(52));
        callback(scada::StatusCode::Good,
                 {scada::AddNodesResult{
                     .status_code = scada::StatusCode::Good,
                     .added_node_id = scada::NodeId{500, 4},
                 }});
      }));

  auto response = WaitAwaitable(executor_, handler_.Handle(std::move(request)));
  const auto* add_nodes_response = std::get_if<AddNodesResponse>(&response);
  ASSERT_NE(add_nodes_response, nullptr);
  EXPECT_EQ(add_nodes_response->status.code(), scada::StatusCode::Good);
  ASSERT_EQ(add_nodes_response->results.size(), 1u);
  EXPECT_EQ(add_nodes_response->results[0].added_node_id, (scada::NodeId{500, 4}));
}

TEST_F(OpcUaWsServiceHandlerTest,
       HandleDeleteAndReferenceMutations_PropagatesStatuses) {
  DeleteNodesRequest delete_nodes_request{
      .items = {{.node_id = NumericNode(60), .delete_target_references = true}}};
  AddReferencesRequest add_references_request{
      .items = {{.source_node_id = NumericNode(61),
                 .reference_type_id = NumericNode(62),
                 .target_node_id = scada::ExpandedNodeId{NumericNode(63)}}}};
  DeleteReferencesRequest delete_references_request{
      .items = {{.source_node_id = NumericNode(64),
                 .reference_type_id = NumericNode(65),
                 .target_node_id = scada::ExpandedNodeId{NumericNode(66)}}}};

  EXPECT_CALL(node_management_service_, DeleteNodes(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::DeleteNodesItem>& items,
                           const scada::DeleteNodesCallback& callback) {
        ASSERT_EQ(items.size(), 1u);
        EXPECT_EQ(items[0].node_id, NumericNode(60));
        EXPECT_TRUE(items[0].delete_target_references);
        callback(scada::StatusCode::Good,
                 {scada::StatusCode::Good, scada::StatusCode::Bad_WrongNodeId});
      }));
  EXPECT_CALL(node_management_service_, AddReferences(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::AddReferencesItem>& items,
                           const scada::AddReferencesCallback& callback) {
        ASSERT_EQ(items.size(), 1u);
        EXPECT_EQ(items[0].source_node_id, NumericNode(61));
        EXPECT_EQ(items[0].reference_type_id, NumericNode(62));
        EXPECT_EQ(items[0].target_node_id,
                  scada::ExpandedNodeId{NumericNode(63)});
        callback(scada::StatusCode::Good,
                 {scada::StatusCode::Good, scada::StatusCode::Bad_WrongTargetId});
      }));
  EXPECT_CALL(node_management_service_, DeleteReferences(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::DeleteReferencesItem>& items,
                           const scada::DeleteReferencesCallback& callback) {
        ASSERT_EQ(items.size(), 1u);
        EXPECT_EQ(items[0].source_node_id, NumericNode(64));
        EXPECT_EQ(items[0].reference_type_id, NumericNode(65));
        EXPECT_EQ(items[0].target_node_id,
                  scada::ExpandedNodeId{NumericNode(66)});
        callback(scada::StatusCode::Bad_Disconnected,
                 {scada::StatusCode::Bad_Disconnected});
      }));

  auto response =
      WaitAwaitable(executor_, handler_.Handle(std::move(delete_nodes_request)));
  const auto* delete_nodes_response =
      std::get_if<DeleteNodesResponse>(&response);
  ASSERT_NE(delete_nodes_response, nullptr);
  EXPECT_EQ(delete_nodes_response->status.code(), scada::StatusCode::Good);
  EXPECT_THAT(delete_nodes_response->results,
              ElementsAre(scada::StatusCode::Good,
                          scada::StatusCode::Bad_WrongNodeId));

  response = WaitAwaitable(executor_,
                           handler_.Handle(std::move(add_references_request)));
  const auto* add_references_response =
      std::get_if<AddReferencesResponse>(&response);
  ASSERT_NE(add_references_response, nullptr);
  EXPECT_EQ(add_references_response->status.code(), scada::StatusCode::Good);
  EXPECT_THAT(add_references_response->results,
              ElementsAre(scada::StatusCode::Good,
                          scada::StatusCode::Bad_WrongTargetId));

  response = WaitAwaitable(executor_,
                           handler_.Handle(std::move(delete_references_request)));
  const auto* delete_references_response =
      std::get_if<DeleteReferencesResponse>(&response);
  ASSERT_NE(delete_references_response, nullptr);
  EXPECT_EQ(delete_references_response->status.code(),
            scada::StatusCode::Bad_Disconnected);
  EXPECT_THAT(delete_references_response->results,
              ElementsAre(scada::StatusCode::Bad_Disconnected));
}

}  // namespace
}  // namespace opcua_ws
