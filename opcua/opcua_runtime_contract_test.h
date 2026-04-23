#pragma once

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "base/time_utils.h"
#include "opcua/opcua_message.h"
#include "opcua/opcua_server_session_manager.h"
#include "scada/attribute_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitoring_parameters.h"
#include "scada/node_management_service_mock.h"
#include "scada/test/test_monitored_item.h"
#include "scada/view_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace opcua::test {

inline scada::NodeId NumericNode(scada::NumericId id,
                                 scada::NamespaceIndex ns = 2) {
  return {id, ns};
}

inline base::Time ParseTime(std::string_view value) {
  base::Time result;
  EXPECT_TRUE(Deserialize(value, result));
  return result;
}

class TestMonitoredItemService : public scada::MonitoredItemService {
 public:
  std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& value_id,
      const scada::MonitoringParameters& params) override {
    created_value_ids.push_back(value_id);
    created_params.push_back(params);
    auto item = std::make_shared<scada::TestMonitoredItem>();
    items.push_back(item);
    return item;
  }

  std::vector<scada::ReadValueId> created_value_ids;
  std::vector<scada::MonitoringParameters> created_params;
  std::vector<std::shared_ptr<scada::TestMonitoredItem>> items;
};

class RuntimeContractTestBase {
 public:
  base::Time now_ = ParseTime("2026-04-22 09:00:00");
  const scada::NodeId expected_user_id_ = NumericNode(700, 5);
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  testing::StrictMock<scada::MockAttributeService> attribute_service_;
  testing::StrictMock<scada::MockViewService> view_service_;
  testing::StrictMock<scada::MockHistoryService> history_service_;
  testing::StrictMock<scada::MockMethodService> method_service_;
  testing::StrictMock<scada::MockNodeManagementService>
      node_management_service_;
  TestMonitoredItemService monitored_item_service_;
  OpcUaSessionManager session_manager_{{
      .authenticator =
          [this](scada::LocalizedText user_name,
                 scada::LocalizedText password)
              -> Awaitable<scada::StatusOr<scada::AuthenticationResult>> {
        EXPECT_EQ(user_name, scada::LocalizedText{u"operator"});
        EXPECT_EQ(password, scada::LocalizedText{u"secret"});
        co_return scada::AuthenticationResult{
            .user_id = expected_user_id_, .multi_sessions = true};
      },
      .now = [this] { return now_; },
  }};
};

template <typename Fixture>
void ExpectRoutesReadRequestsThroughActivatedSessionUser(Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  fixture.CreateAndActivate(connection);

  ReadRequest request{
      .inputs = {{.node_id = NumericNode(1),
                  .attribute_id = scada::AttributeId::DisplayName}}};
  EXPECT_CALL(fixture.attribute_service_, Read(testing::_, testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [&](const scada::ServiceContext& context,
              const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
              const scada::ReadCallback& callback) {
            EXPECT_EQ(context.user_id(), fixture.expected_user_id_);
            EXPECT_THAT(*inputs, testing::ElementsAre(request.inputs[0]));
            callback(scada::StatusCode::Good,
                     {scada::DataValue{scada::LocalizedText{u"Pump"}, {},
                                       fixture.now_, fixture.now_}});
          }));

  const auto response =
      fixture.template HandleResponse<ReadResponse>(connection, request);
  EXPECT_EQ(response.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(response.results.size(), 1u);
  EXPECT_EQ(response.results[0].value,
            scada::Variant{scada::LocalizedText{u"Pump"}});
}

template <typename Fixture>
void ExpectRoutesWriteRequestsThroughActivatedSessionUser(Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  fixture.CreateAndActivate(connection);

  WriteRequest request;
  request.inputs.push_back(scada::WriteValue{
      .node_id = NumericNode(2),
      .attribute_id = scada::AttributeId::Value,
      .value = scada::Variant{17.5},
  });
  EXPECT_CALL(fixture.attribute_service_,
              Write(testing::_, testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [&](const scada::ServiceContext& context,
              const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
              const scada::WriteCallback& callback) {
            EXPECT_EQ(context.user_id(), fixture.expected_user_id_);
            ASSERT_EQ(inputs->size(), 1u);
            EXPECT_EQ((*inputs)[0].node_id, request.inputs[0].node_id);
            EXPECT_EQ((*inputs)[0].attribute_id, request.inputs[0].attribute_id);
            EXPECT_EQ((*inputs)[0].value, request.inputs[0].value);
            callback(scada::StatusCode::Good,
                     std::vector<scada::StatusCode>{
                         scada::StatusCode::Good_Manual});
          }));

  const auto response =
      fixture.template HandleResponse<WriteResponse>(connection, request);
  EXPECT_EQ(response.status.code(), scada::StatusCode::Good);
  EXPECT_THAT(response.results,
              testing::ElementsAre(
                  static_cast<scada::StatusCode>(scada::StatusCode::Good_Manual)));
}

template <typename Fixture>
void ExpectRoutesCallRequestsThroughActivatedSessionUser(Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  fixture.CreateAndActivate(connection);

  CallRequest request{
      .methods = {{.object_id = NumericNode(301),
                   .method_id = NumericNode(302),
                   .arguments = {scada::Variant{7.0},
                                 scada::Variant{std::string{"mode"}}}}}};
  EXPECT_CALL(fixture.method_service_,
              Call(request.methods[0].object_id, request.methods[0].method_id,
                   request.methods[0].arguments, fixture.expected_user_id_,
                   testing::_))
      .WillOnce(testing::Invoke(
          [](const scada::NodeId&,
             const scada::NodeId&,
             const std::vector<scada::Variant>&,
             const scada::NodeId&,
             const scada::StatusCallback& callback) {
            callback(scada::StatusCode::Good);
          }));

  const auto response =
      fixture.template HandleResponse<CallResponse>(connection, request);
  ASSERT_EQ(response.results.size(), 1u);
  EXPECT_EQ(response.results[0].status.code(), scada::StatusCode::Good);
  EXPECT_TRUE(response.results[0].input_argument_results.empty());
  EXPECT_TRUE(response.results[0].output_arguments.empty());
}

template <typename Fixture>
void ExpectHistoryReadRawPreservesPayloadThroughActivatedSession(
    Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  fixture.CreateAndActivate(connection);

  const auto from = fixture.now_ - base::TimeDelta::FromMinutes(15);
  const auto to = fixture.now_;
  HistoryReadRawRequest request{
      .details = {.node_id = NumericNode(401),
                  .from = from,
                  .to = to,
                  .max_count = 3}};
  EXPECT_CALL(fixture.history_service_, HistoryReadRaw(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [&](const scada::HistoryReadRawDetails& details,
              const scada::HistoryReadRawCallback& callback) {
            EXPECT_EQ(details.node_id, request.details.node_id);
            EXPECT_EQ(details.from, from);
            EXPECT_EQ(details.to, to);
            EXPECT_EQ(details.max_count, 3u);
            callback(scada::HistoryReadRawResult{
                .status = scada::StatusCode::Good,
                .values = {scada::DataValue{
                    scada::Variant{12.5}, {}, fixture.now_, fixture.now_}},
                .continuation_point = {1, 2, 3},
            });
          }));

  const auto response = fixture.template HandleResponse<HistoryReadRawResponse>(
      connection, request);
  EXPECT_EQ(response.result.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(response.result.values.size(), 1u);
  EXPECT_EQ(response.result.values[0].value, scada::Variant{12.5});
  EXPECT_EQ(response.result.continuation_point, (scada::ByteString{1, 2, 3}));
}

template <typename Fixture>
void ExpectHistoryReadEventsPreservesPayloadThroughActivatedSession(
    Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  fixture.CreateAndActivate(connection);

  const auto from = fixture.now_ - base::TimeDelta::FromHours(4);
  const auto to = fixture.now_;
  HistoryReadEventsRequest request{
      .details = {.node_id = NumericNode(402),
                  .from = from,
                  .to = to,
                  .filter = {}}};
  EXPECT_CALL(fixture.history_service_,
              HistoryReadEvents(testing::_, testing::_, testing::_, testing::_,
                                testing::_))
      .WillOnce(testing::Invoke(
          [&](const scada::NodeId& node_id,
              base::Time actual_from,
              base::Time actual_to,
              const scada::EventFilter&,
              const scada::HistoryReadEventsCallback& callback) {
            EXPECT_EQ(node_id, request.details.node_id);
            EXPECT_EQ(actual_from, from);
            EXPECT_EQ(actual_to, to);
            scada::Event event;
            event.event_id = 99;
            event.time = fixture.now_;
            event.receive_time = fixture.now_;
            event.node_id = NumericNode(403);
            event.message = scada::LocalizedText{u"alarm"};
            callback(scada::HistoryReadEventsResult{
                .status = scada::StatusCode::Good,
                .events = {std::move(event)},
            });
          }));

  const auto response =
      fixture.template HandleResponse<HistoryReadEventsResponse>(connection,
                                                                 request);
  EXPECT_EQ(response.result.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(response.result.events.size(), 1u);
  EXPECT_EQ(response.result.events[0].event_id, 99u);
  EXPECT_EQ(response.result.events[0].node_id, NumericNode(403));
}

template <typename Fixture>
void ExpectNodeManagementMutationsPreserveBatchResults(Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  fixture.CreateAndActivate(connection);

  AddNodesRequest add_nodes{
      .items = {{.requested_id = NumericNode(501),
                 .parent_id = NumericNode(502),
                 .type_definition_id = NumericNode(503)}}};
  DeleteNodesRequest delete_nodes{
      .items = {{.node_id = NumericNode(504), .delete_target_references = true}}};
  AddReferencesRequest add_references{
      .items = {{.source_node_id = NumericNode(505),
                 .reference_type_id = NumericNode(506),
                 .target_node_id = scada::ExpandedNodeId{NumericNode(507)}}}};
  DeleteReferencesRequest delete_references{
      .items = {{.source_node_id = NumericNode(508),
                 .reference_type_id = NumericNode(509),
                 .target_node_id = scada::ExpandedNodeId{NumericNode(510)}}}};

  EXPECT_CALL(fixture.node_management_service_, AddNodes(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [&](const std::vector<scada::AddNodesItem>& items,
              const scada::AddNodesCallback& callback) {
            ASSERT_EQ(items.size(), 1u);
            EXPECT_EQ(items[0].requested_id, add_nodes.items[0].requested_id);
            EXPECT_EQ(items[0].parent_id, add_nodes.items[0].parent_id);
            EXPECT_EQ(items[0].type_definition_id,
                      add_nodes.items[0].type_definition_id);
            callback(scada::StatusCode::Good,
                     {scada::AddNodesResult{
                         .status_code = scada::StatusCode::Good,
                         .added_node_id = NumericNode(511),
                     }});
          }));
  EXPECT_CALL(fixture.node_management_service_,
              DeleteNodes(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [&](const std::vector<scada::DeleteNodesItem>& items,
              const scada::DeleteNodesCallback& callback) {
            ASSERT_EQ(items.size(), 1u);
            EXPECT_EQ(items[0].node_id, delete_nodes.items[0].node_id);
            EXPECT_TRUE(items[0].delete_target_references);
            callback(scada::StatusCode::Good,
                     {scada::StatusCode::Good,
                      scada::StatusCode::Bad_WrongNodeId});
          }));
  EXPECT_CALL(fixture.node_management_service_,
              AddReferences(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [&](const std::vector<scada::AddReferencesItem>& items,
              const scada::AddReferencesCallback& callback) {
            ASSERT_EQ(items.size(), 1u);
            EXPECT_EQ(items[0].source_node_id,
                      add_references.items[0].source_node_id);
            EXPECT_EQ(items[0].reference_type_id,
                      add_references.items[0].reference_type_id);
            EXPECT_EQ(items[0].target_node_id,
                      add_references.items[0].target_node_id);
            callback(scada::StatusCode::Good,
                     {scada::StatusCode::Good,
                      scada::StatusCode::Bad_WrongTargetId});
          }));
  EXPECT_CALL(fixture.node_management_service_,
              DeleteReferences(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [&](const std::vector<scada::DeleteReferencesItem>& items,
              const scada::DeleteReferencesCallback& callback) {
            ASSERT_EQ(items.size(), 1u);
            EXPECT_EQ(items[0].source_node_id,
                      delete_references.items[0].source_node_id);
            EXPECT_EQ(items[0].reference_type_id,
                      delete_references.items[0].reference_type_id);
            EXPECT_EQ(items[0].target_node_id,
                      delete_references.items[0].target_node_id);
            callback(scada::StatusCode::Bad_Disconnected,
                     {scada::StatusCode::Bad_Disconnected});
          }));

  const auto add_nodes_response =
      fixture.template HandleResponse<AddNodesResponse>(connection, add_nodes);
  EXPECT_EQ(add_nodes_response.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(add_nodes_response.results.size(), 1u);
  EXPECT_EQ(add_nodes_response.results[0].status_code,
            scada::StatusCode::Good);
  EXPECT_EQ(add_nodes_response.results[0].added_node_id, NumericNode(511));

  const auto delete_nodes_response = fixture.template HandleResponse<
      DeleteNodesResponse>(connection, delete_nodes);
  EXPECT_EQ(delete_nodes_response.status.code(), scada::StatusCode::Good);
  EXPECT_THAT(delete_nodes_response.results,
              testing::ElementsAre(scada::StatusCode::Good,
                                   scada::StatusCode::Bad_WrongNodeId));

  const auto add_references_response = fixture.template HandleResponse<
      AddReferencesResponse>(connection, add_references);
  EXPECT_EQ(add_references_response.status.code(), scada::StatusCode::Good);
  EXPECT_THAT(add_references_response.results,
              testing::ElementsAre(scada::StatusCode::Good,
                                   scada::StatusCode::Bad_WrongTargetId));

  const auto delete_references_response = fixture.template HandleResponse<
      DeleteReferencesResponse>(connection, delete_references);
  EXPECT_EQ(delete_references_response.status.code(),
            scada::StatusCode::Bad_Disconnected);
  EXPECT_THAT(delete_references_response.results,
              testing::ElementsAre(scada::StatusCode::Bad_Disconnected));
}

template <typename Fixture>
void ExpectPreservesLiveSubscriptionStateAcrossDetachAndResume(
    Fixture& fixture) {
  typename Fixture::ConnectionState first_connection;
  const auto [session_id, authentication_token] =
      fixture.CreateAndActivate(first_connection);

  const auto subscription =
      fixture.template HandleResponse<OpcUaCreateSubscriptionResponse>(
          first_connection,
          OpcUaCreateSubscriptionRequest{
              .parameters = {.publishing_interval_ms = 100,
                             .lifetime_count = 60,
                             .max_keep_alive_count = 3,
                             .publishing_enabled = true}});
  ASSERT_EQ(subscription.status.code(), scada::StatusCode::Good);

  const auto create_items =
      fixture.template HandleResponse<OpcUaCreateMonitoredItemsResponse>(
          first_connection,
          OpcUaCreateMonitoredItemsRequest{
              .subscription_id = subscription.subscription_id,
              .items_to_create = {{.item_to_monitor =
                                       {.node_id = NumericNode(11),
                                        .attribute_id =
                                            scada::AttributeId::Value},
                                   .requested_parameters =
                                       {.client_handle = 44,
                                        .sampling_interval_ms = 0,
                                        .queue_size = 1,
                                        .discard_oldest = true}}}});
  ASSERT_EQ(create_items.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(fixture.monitored_item_service_.items.size(), 1u);

  fixture.monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{12.5}, {}, fixture.now_, fixture.now_});
  fixture.Detach(first_connection);
  EXPECT_FALSE(first_connection.authentication_token.has_value());

  typename Fixture::ConnectionState second_connection;
  const auto resumed =
      fixture.template HandleResponse<OpcUaActivateSessionResponse>(
          second_connection,
          OpcUaActivateSessionRequest{
              .session_id = session_id,
              .authentication_token = authentication_token,
          });
  EXPECT_EQ(resumed.status.code(), scada::StatusCode::Good);
  EXPECT_TRUE(resumed.resumed);

  fixture.now_ = fixture.now_ + base::TimeDelta::FromMilliseconds(100);
  const auto publish = fixture.template HandleResponse<OpcUaPublishResponse>(
      second_connection, OpcUaPublishRequest{});
  EXPECT_EQ(publish.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(publish.subscription_id, subscription.subscription_id);
  const auto* data = std::get_if<OpcUaDataChangeNotification>(
      &publish.notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].value.value.get<double>(), 12.5);
}

template <typename Fixture>
void ExpectTransfersSubscriptionsAcrossSessions(Fixture& fixture) {
  typename Fixture::ConnectionState source_connection;
  fixture.CreateAndActivate(source_connection);

  const auto created_subscription =
      fixture.template HandleResponse<OpcUaCreateSubscriptionResponse>(
          source_connection,
          OpcUaCreateSubscriptionRequest{
              .parameters = {.publishing_interval_ms = 100,
                             .lifetime_count = 60,
                             .max_keep_alive_count = 3,
                             .publishing_enabled = true}});
  ASSERT_EQ(created_subscription.status.code(), scada::StatusCode::Good);

  const auto created_items =
      fixture.template HandleResponse<OpcUaCreateMonitoredItemsResponse>(
          source_connection,
          OpcUaCreateMonitoredItemsRequest{
              .subscription_id = created_subscription.subscription_id,
              .items_to_create = {{.item_to_monitor =
                                       {.node_id = NumericNode(21),
                                        .attribute_id =
                                            scada::AttributeId::Value},
                                   .requested_parameters =
                                       {.client_handle = 55,
                                        .sampling_interval_ms = 0,
                                        .queue_size = 1,
                                        .discard_oldest = true}}}});
  ASSERT_EQ(created_items.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(fixture.monitored_item_service_.items.size(), 1u);

  fixture.monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{77.0}, {}, fixture.now_, fixture.now_});

  typename Fixture::ConnectionState target_connection;
  fixture.CreateAndActivate(target_connection);
  const auto transferred =
      fixture.template HandleResponse<OpcUaTransferSubscriptionsResponse>(
          target_connection,
          OpcUaTransferSubscriptionsRequest{
              .subscription_ids = {created_subscription.subscription_id},
              .send_initial_values = true});
  EXPECT_EQ(transferred.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  const auto source_publish = fixture.template HandleResponse<OpcUaPublishResponse>(
      source_connection, OpcUaPublishRequest{});
  EXPECT_EQ(source_publish.status.code(), scada::StatusCode::Bad_NothingToDo);

  fixture.now_ = fixture.now_ + base::TimeDelta::FromMilliseconds(100);
  const auto target_publish = fixture.template HandleResponse<OpcUaPublishResponse>(
      target_connection, OpcUaPublishRequest{});
  EXPECT_EQ(target_publish.subscription_id,
            created_subscription.subscription_id);
  const auto* data = std::get_if<OpcUaDataChangeNotification>(
      &target_publish.notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].value.value.get<double>(), 77.0);
}

template <typename Fixture>
void ExpectCloseSessionClearsAttachedState(Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  const auto [session_id, authentication_token] =
      fixture.CreateAndActivate(connection);

  const auto closed = fixture.template HandleResponse<OpcUaCloseSessionResponse>(
      connection,
      OpcUaCloseSessionRequest{
          .session_id = session_id,
          .authentication_token = authentication_token,
      });
  EXPECT_EQ(closed.status.code(), scada::StatusCode::Good);
  EXPECT_FALSE(connection.authentication_token.has_value());

  const auto status = fixture.ReadStatus(
      connection,
      ReadRequest{.inputs = {{.node_id = NumericNode(31),
                              .attribute_id = scada::AttributeId::Value}}});
  EXPECT_EQ(status, scada::StatusCode::Bad_SessionIsLoggedOff);
}

template <typename Fixture>
void ExpectRejectsHistoryReadRawWithoutActivatedSession(Fixture& fixture) {
  typename Fixture::ConnectionState connection;

  const auto status = fixture.HistoryReadRawStatus(
      connection,
      HistoryReadRawRequest{
          .details =
              {.node_id = NumericNode(41),
               .from = fixture.now_ - base::TimeDelta::FromMinutes(10),
               .to = fixture.now_,
               .max_count = 5}});
  EXPECT_EQ(status, scada::StatusCode::Bad_SessionIsLoggedOff);
}

template <typename Fixture>
void ExpectRejectsHistoryReadEventsWithoutActivatedSession(Fixture& fixture) {
  typename Fixture::ConnectionState connection;

  const auto status = fixture.HistoryReadEventsStatus(
      connection,
      HistoryReadEventsRequest{
          .details =
              {.node_id = NumericNode(42),
               .from = fixture.now_ - base::TimeDelta::FromMinutes(30),
               .to = fixture.now_}});
  EXPECT_EQ(status, scada::StatusCode::Bad_SessionIsLoggedOff);
}

template <typename Fixture>
void ExpectBrowseAndBrowseNextUseSessionScopedContinuationPoints(
    Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  fixture.CreateAndActivate(connection);

  EXPECT_CALL(fixture.view_service_, Browse(testing::_, testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [&](const scada::ServiceContext& context,
              const std::vector<scada::BrowseDescription>& inputs,
              const scada::BrowseCallback& callback) {
            EXPECT_EQ(context.user_id(), fixture.expected_user_id_);
            ASSERT_EQ(inputs.size(), 1u);
            callback(scada::StatusCode::Good,
                     {scada::BrowseResult{
                         .status_code = scada::StatusCode::Good,
                         .references = {
                             {.reference_type_id = NumericNode(901),
                              .forward = true,
                              .node_id = NumericNode(902)},
                             {.reference_type_id = NumericNode(903),
                              .forward = false,
                              .node_id = NumericNode(904)},
                             {.reference_type_id = NumericNode(905),
                              .forward = true,
                              .node_id = NumericNode(906)}}}});
          }));

  const auto browse = fixture.template HandleResponse<BrowseResponse>(
      connection,
      BrowseRequest{
          .requested_max_references_per_node = 2,
          .inputs = {{.node_id = NumericNode(900),
                      .direction = scada::BrowseDirection::Both,
                      .reference_type_id = NumericNode(910),
                      .include_subtypes = true}}});
  ASSERT_EQ(browse.results.size(), 1u);
  ASSERT_EQ(browse.results[0].references.size(), 2u);
  ASSERT_FALSE(browse.results[0].continuation_point.empty());
  EXPECT_EQ(browse.results[0].references[0].node_id, NumericNode(902));
  EXPECT_EQ(browse.results[0].references[1].node_id, NumericNode(904));

  typename Fixture::ConnectionState other_connection;
  fixture.CreateAndActivate(other_connection);
  const auto wrong_session = fixture.template HandleResponse<BrowseNextResponse>(
      other_connection,
      BrowseNextRequest{
          .continuation_points = {browse.results[0].continuation_point}});
  ASSERT_EQ(wrong_session.results.size(), 1u);
  EXPECT_EQ(wrong_session.results[0].status_code,
            scada::StatusCode::Bad_WrongIndex);

  const auto browse_next = fixture.template HandleResponse<BrowseNextResponse>(
      connection,
      BrowseNextRequest{
          .continuation_points = {browse.results[0].continuation_point}});
  ASSERT_EQ(browse_next.results.size(), 1u);
  EXPECT_EQ(browse_next.results[0].status_code, scada::StatusCode::Good);
  ASSERT_EQ(browse_next.results[0].references.size(), 1u);
  EXPECT_EQ(browse_next.results[0].references[0].node_id, NumericNode(906));
  EXPECT_TRUE(browse_next.results[0].continuation_point.empty());

  const auto invalid = fixture.template HandleResponse<BrowseNextResponse>(
      connection,
      BrowseNextRequest{
          .continuation_points = {browse.results[0].continuation_point}});
  ASSERT_EQ(invalid.results.size(), 1u);
  EXPECT_EQ(invalid.results[0].status_code, scada::StatusCode::Bad_WrongIndex);
}

template <typename Fixture>
void ExpectPublishReturnsKeepAliveWhenNoNotifications(Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  fixture.CreateAndActivate(connection);

  const auto created_subscription =
      fixture.template HandleResponse<OpcUaCreateSubscriptionResponse>(
          connection,
          OpcUaCreateSubscriptionRequest{
              .parameters = {.publishing_interval_ms = 100,
                             .lifetime_count = 60,
                             .max_keep_alive_count = 3,
                             .publishing_enabled = true}});
  ASSERT_EQ(created_subscription.status.code(), scada::StatusCode::Good);

  fixture.now_ = fixture.now_ + base::TimeDelta::FromMilliseconds(300);
  const auto publish = fixture.template HandleResponse<OpcUaPublishResponse>(
      connection, OpcUaPublishRequest{});
  EXPECT_EQ(publish.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(publish.subscription_id, created_subscription.subscription_id);
  EXPECT_TRUE(publish.notification_message.notification_data.empty());
  EXPECT_EQ(publish.notification_message.sequence_number, 1u);
  EXPECT_TRUE(publish.available_sequence_numbers.empty());
}

template <typename Fixture>
void ExpectRepublishReplaysNotificationUntilAcknowledged(Fixture& fixture) {
  typename Fixture::ConnectionState connection;
  fixture.CreateAndActivate(connection);

  const auto created_subscription =
      fixture.template HandleResponse<OpcUaCreateSubscriptionResponse>(
          connection,
          OpcUaCreateSubscriptionRequest{
              .parameters = {.publishing_interval_ms = 100,
                             .lifetime_count = 60,
                             .max_keep_alive_count = 3,
                             .publishing_enabled = true}});
  ASSERT_EQ(created_subscription.status.code(), scada::StatusCode::Good);

  const auto create_items =
      fixture.template HandleResponse<OpcUaCreateMonitoredItemsResponse>(
          connection,
          OpcUaCreateMonitoredItemsRequest{
              .subscription_id = created_subscription.subscription_id,
              .items_to_create = {{.item_to_monitor =
                                       {.node_id = NumericNode(51),
                                        .attribute_id =
                                            scada::AttributeId::Value},
                                   .requested_parameters =
                                       {.client_handle = 88,
                                        .sampling_interval_ms = 0,
                                        .queue_size = 1,
                                        .discard_oldest = true}}}});
  ASSERT_EQ(create_items.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(fixture.monitored_item_service_.items.size(), 1u);

  fixture.monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{42.5}, {}, fixture.now_, fixture.now_});
  fixture.now_ = fixture.now_ + base::TimeDelta::FromMilliseconds(100);

  const auto publish = fixture.template HandleResponse<OpcUaPublishResponse>(
      connection, OpcUaPublishRequest{});
  EXPECT_EQ(publish.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(publish.subscription_id, created_subscription.subscription_id);
  EXPECT_EQ(publish.available_sequence_numbers,
            (std::vector<scada::UInt32>{1u}));
  ASSERT_EQ(publish.notification_message.notification_data.size(), 1u);
  const auto* published_data = std::get_if<OpcUaDataChangeNotification>(
      &publish.notification_message.notification_data[0]);
  ASSERT_NE(published_data, nullptr);
  ASSERT_EQ(published_data->monitored_items.size(), 1u);
  EXPECT_EQ(published_data->monitored_items[0].client_handle, 88u);
  EXPECT_EQ(published_data->monitored_items[0].value.value.get<double>(), 42.5);

  const auto republish = fixture.template HandleResponse<OpcUaRepublishResponse>(
      connection,
      OpcUaRepublishRequest{
          .subscription_id = created_subscription.subscription_id,
          .retransmit_sequence_number =
              publish.notification_message.sequence_number});
  EXPECT_EQ(republish.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(republish.notification_message, publish.notification_message);

  fixture.now_ = fixture.now_ + base::TimeDelta::FromMilliseconds(300);
  const auto ack_publish = fixture.template HandleResponse<OpcUaPublishResponse>(
      connection,
      OpcUaPublishRequest{
          .subscription_acknowledgements = {{
              .subscription_id = created_subscription.subscription_id,
              .sequence_number = publish.notification_message.sequence_number,
          }}});
  EXPECT_EQ(ack_publish.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(ack_publish.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));
  EXPECT_TRUE(ack_publish.available_sequence_numbers.empty());

  const auto after_ack =
      fixture.template HandleResponse<OpcUaRepublishResponse>(
          connection,
          OpcUaRepublishRequest{
              .subscription_id = created_subscription.subscription_id,
              .retransmit_sequence_number =
                  publish.notification_message.sequence_number});
  EXPECT_EQ(after_ack.status.code(), scada::StatusCode::Bad_MessageNotAvailable);
}

}  // namespace opcua::test
