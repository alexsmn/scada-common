#include "opcua/service_handler.h"

#include "base/test/awaitable_test.h"
#include "base/any_executor.h"
#include "base/test/test_executor.h"
#include "scada/attribute_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/node_management_service_mock.h"
#include "scada/service_context.h"
#include "scada/view_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>

using namespace testing;

namespace opcua {
namespace {

class ServiceHandlerTest : public Test {
 protected:
  static scada::NodeId NumericNode(scada::NumericId id,
                                   scada::NamespaceIndex ns = 2) {
    return {id, ns};
  }

  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockViewService> view_service_;
  StrictMock<scada::MockHistoryService> history_service_;
  StrictMock<scada::MockMethodService> method_service_;
  StrictMock<scada::MockNodeManagementService> node_management_service_;
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  const AnyExecutor any_executor_ = MakeTestAnyExecutor(executor_);
  const scada::NodeId user_id_ = NumericNode(700, 3);
  ServiceHandler handler_{
      {any_executor_,
       attribute_service_,
       view_service_,
       history_service_,
       method_service_,
       node_management_service_,
       user_id_}};
};

TEST_F(ServiceHandlerTest,
       HandleReadWriteBrowseAndTranslate_UsesPhase0Services) {
  ReadRequest read_request{
      .inputs = {{.node_id = NumericNode(1),
                  .attribute_id = scada::AttributeId::DisplayName}}};
  WriteRequest write_request{
      .inputs = {{.node_id = NumericNode(2),
                  .attribute_id = scada::AttributeId::Value,
                  .value = scada::Variant{scada::Int32{7}},
                  .flags = scada::WriteFlags{}.set_select()}}};
  BrowseRequest browse_request{
      .inputs = {{.node_id = NumericNode(3),
                  .direction = scada::BrowseDirection::Forward,
                  .reference_type_id = NumericNode(33),
                  .include_subtypes = false}}};
  TranslateBrowsePathsRequest translate_request{
      .inputs = {{.node_id = NumericNode(4),
                  .relative_path = {{.reference_type_id = NumericNode(44),
                                     .inverse = true,
                                     .include_subtypes = false,
                                     .target_name = {"Leaf", 5}}}}}};

  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(context.user_id(), user_id_);
        ASSERT_EQ(inputs->size(), 1u);
        EXPECT_EQ((*inputs)[0], read_request.inputs[0]);
        callback(scada::StatusCode::Good,
                 {scada::DataValue{scada::LocalizedText{u"Pump"}, {},
                                   base::Time::Now(), base::Time::Now()}});
      }));
  EXPECT_CALL(attribute_service_, Write(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
                           const scada::WriteCallback& callback) {
        EXPECT_EQ(context.user_id(), user_id_);
        ASSERT_EQ(inputs->size(), 1u);
        EXPECT_EQ((*inputs)[0], write_request.inputs[0]);
        callback(scada::StatusCode::Good, {scada::StatusCode::Good});
      }));
  EXPECT_CALL(view_service_, Browse(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::vector<scada::BrowseDescription>& inputs,
                           const scada::BrowseCallback& callback) {
        EXPECT_EQ(context.user_id(), user_id_);
        EXPECT_THAT(inputs, ElementsAre(browse_request.inputs[0]));
        callback(scada::StatusCode::Good,
                 {scada::BrowseResult{
                     .status_code = scada::StatusCode::Good,
                     .references = {{.reference_type_id = NumericNode(34),
                                     .forward = true,
                                     .node_id = NumericNode(35)}}}});
      }));
  EXPECT_CALL(view_service_, TranslateBrowsePaths(_, _))
      .WillOnce(Invoke([&](const std::vector<scada::BrowsePath>& inputs,
                           const scada::TranslateBrowsePathsCallback& callback) {
        EXPECT_THAT(inputs, ElementsAre(translate_request.inputs[0]));
        callback(scada::StatusCode::Good,
                 {scada::BrowsePathResult{
                     .status_code = scada::StatusCode::Good,
                     .targets = {{.target_id = scada::ExpandedNodeId{NumericNode(45)},
                                  .remaining_path_index = 0}}}});
      }));

  auto response = WaitAwaitable(executor_, handler_.Handle(read_request));
  const auto* read_response = std::get_if<ReadResponse>(&response);
  ASSERT_NE(read_response, nullptr);
  EXPECT_EQ(read_response->status.code(), scada::StatusCode::Good);
  ASSERT_EQ(read_response->results.size(), 1u);
  EXPECT_EQ(read_response->results[0].value,
            scada::Variant{scada::LocalizedText{u"Pump"}});

  response = WaitAwaitable(executor_, handler_.Handle(write_request));
  const auto* write_response = std::get_if<WriteResponse>(&response);
  ASSERT_NE(write_response, nullptr);
  EXPECT_EQ(write_response->status.code(), scada::StatusCode::Good);
  EXPECT_THAT(write_response->results, ElementsAre(scada::StatusCode::Good));

  response = WaitAwaitable(executor_, handler_.Handle(browse_request));
  const auto* browse_response = std::get_if<BrowseResponse>(&response);
  ASSERT_NE(browse_response, nullptr);
  EXPECT_EQ(browse_response->status.code(), scada::StatusCode::Good);
  ASSERT_EQ(browse_response->results.size(), 1u);
  EXPECT_THAT(browse_response->results[0].references,
              ElementsAre(scada::ReferenceDescription{
                  .reference_type_id = NumericNode(34),
                  .forward = true,
                  .node_id = NumericNode(35)}));

  response = WaitAwaitable(executor_, handler_.Handle(translate_request));
  const auto* translate_response =
      std::get_if<TranslateBrowsePathsResponse>(&response);
  ASSERT_NE(translate_response, nullptr);
  EXPECT_EQ(translate_response->status.code(), scada::StatusCode::Good);
  ASSERT_EQ(translate_response->results.size(), 1u);
  ASSERT_EQ(translate_response->results[0].targets.size(), 1u);
  EXPECT_EQ(translate_response->results[0].targets[0].target_id,
            scada::ExpandedNodeId{NumericNode(45)});
  EXPECT_EQ(translate_response->results[0].targets[0].remaining_path_index, 0u);
}

TEST_F(ServiceHandlerTest,
       HandleRead_MapsWrongNodeIdToBadNodeIdUnknown) {
  ReadRequest read_request{
      .inputs = {{.node_id = NumericNode(9999),
                  .attribute_id = scada::AttributeId::Value}}};

  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(context.user_id(), user_id_);
        ASSERT_EQ(inputs->size(), 1u);
        EXPECT_EQ((*inputs)[0], read_request.inputs[0]);
        callback(scada::StatusCode::Good,
                 {scada::MakeReadError(scada::StatusCode::Bad_WrongNodeId)});
      }));

  const auto response = WaitAwaitable(executor_, handler_.Handle(read_request));
  const auto* read_response = std::get_if<ReadResponse>(&response);
  ASSERT_NE(read_response, nullptr);
  ASSERT_EQ(read_response->results.size(), 1u);
  EXPECT_EQ(read_response->status.code(), scada::StatusCode::Good);
  EXPECT_EQ(scada::Status(read_response->results[0].status_code).full_code(),
            0x80340000u);
}

TEST_F(ServiceHandlerTest,
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

TEST_F(ServiceHandlerTest, HandleHistoryReadRaw_PreservesResultPayload) {
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

TEST_F(ServiceHandlerTest,
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

TEST_F(ServiceHandlerTest, HandleAddNodes_ForwardsBatchResults) {
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

TEST_F(ServiceHandlerTest,
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
}  // namespace opcua
