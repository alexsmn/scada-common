#include "opcua/opcua_binary_runtime.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "base/time_utils.h"
#include "scada/attribute_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitoring_parameters.h"
#include "scada/node_management_service_mock.h"
#include "scada/test/test_monitored_item.h"
#include "scada/view_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace opcua {
namespace {

scada::NodeId NumericNode(scada::NumericId id, scada::NamespaceIndex ns = 2) {
  return {id, ns};
}

base::Time ParseTime(std::string_view value) {
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

class OpcUaBinaryRuntimeTest : public Test {
 protected:
  template <typename Response, typename Request>
  Response Handle(OpcUaBinaryConnectionState& connection, Request request) {
    return WaitAwaitable(executor_,
                         runtime_->Handle<Response>(connection,
                                                    std::move(request)));
  }

  std::pair<scada::NodeId, scada::NodeId> CreateAndActivate(
      OpcUaBinaryConnectionState& connection) {
    const auto created = WaitAwaitable(
        executor_, runtime_->CreateSession(OpcUaBinaryCreateSessionRequest{}));
    EXPECT_EQ(created.status.code(), scada::StatusCode::Good);

    const auto activated = WaitAwaitable(
        executor_,
        runtime_->ActivateSession(
            connection, OpcUaBinaryActivateSessionRequest{
                            .session_id = created.session_id,
                            .authentication_token = created.authentication_token,
                            .user_name = scada::LocalizedText{u"operator"},
                            .password = scada::LocalizedText{u"secret"},
                        }));
    EXPECT_EQ(activated.status.code(), scada::StatusCode::Good);
    EXPECT_FALSE(activated.resumed);
    return {created.session_id, created.authentication_token};
  }

  base::Time now_ = ParseTime("2026-04-21 09:00:00");
  const scada::NodeId expected_user_id_ = NumericNode(700, 5);
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockViewService> view_service_;
  StrictMock<scada::MockHistoryService> history_service_;
  StrictMock<scada::MockMethodService> method_service_;
  StrictMock<scada::MockNodeManagementService> node_management_service_;
  TestMonitoredItemService monitored_item_service_;
  opcua_ws::OpcUaWsSessionManager session_manager_{{
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
  std::unique_ptr<OpcUaBinaryRuntime> runtime_ =
      std::make_unique<OpcUaBinaryRuntime>(OpcUaBinaryRuntimeContext{
          .executor = executor_,
          .session_manager = session_manager_,
          .monitored_item_service = monitored_item_service_,
          .attribute_service = attribute_service_,
          .view_service = view_service_,
          .history_service = history_service_,
          .method_service = method_service_,
          .node_management_service = node_management_service_,
          .now = [this] { return now_; },
      });
};

TEST_F(OpcUaBinaryRuntimeTest, RoutesReadRequestsThroughActivatedSessionUser) {
  OpcUaBinaryConnectionState connection;
  CreateAndActivate(connection);

  OpcUaBinaryReadRequest request{
      .inputs = {{.node_id = NumericNode(1),
                  .attribute_id = scada::AttributeId::DisplayName}}};
  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(context.user_id(), expected_user_id_);
        EXPECT_THAT(*inputs, ElementsAre(request.inputs[0]));
        callback(scada::StatusCode::Good,
                 {scada::DataValue{scada::LocalizedText{u"Pump"}, {}, now_,
                                   now_}});
      }));

  const auto response = Handle<OpcUaBinaryReadResponse>(connection, request);
  EXPECT_EQ(response.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(response.results.size(), 1u);
  EXPECT_EQ(response.results[0].value,
            scada::Variant{scada::LocalizedText{u"Pump"}});
}

TEST_F(OpcUaBinaryRuntimeTest,
       PreservesLiveSubscriptionStateAcrossDetachAndResume) {
  OpcUaBinaryConnectionState first_connection;
  const auto [session_id, authentication_token] =
      CreateAndActivate(first_connection);

  const auto subscription = Handle<OpcUaBinaryCreateSubscriptionResponse>(
      first_connection,
      OpcUaBinaryCreateSubscriptionRequest{
          .parameters = {.publishing_interval_ms = 100,
                         .lifetime_count = 60,
                         .max_keep_alive_count = 3,
                         .publishing_enabled = true}});
  ASSERT_EQ(subscription.status.code(), scada::StatusCode::Good);

  const auto create_items =
      Handle<OpcUaBinaryCreateMonitoredItemsResponse>(
          first_connection,
          OpcUaBinaryCreateMonitoredItemsRequest{
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
  ASSERT_EQ(monitored_item_service_.items.size(), 1u);

  monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{12.5}, {}, now_, now_});
  runtime_->Detach(first_connection);
  EXPECT_FALSE(first_connection.authentication_token.has_value());

  OpcUaBinaryConnectionState second_connection;
  const auto resumed = WaitAwaitable(
      executor_,
      runtime_->ActivateSession(second_connection,
                                OpcUaBinaryActivateSessionRequest{
                                    .session_id = session_id,
                                    .authentication_token =
                                        authentication_token,
                                }));
  EXPECT_EQ(resumed.status.code(), scada::StatusCode::Good);
  EXPECT_TRUE(resumed.resumed);

  now_ = now_ + base::TimeDelta::FromMilliseconds(100);
  const auto publish = Handle<OpcUaBinaryPublishResponse>(
      second_connection, OpcUaBinaryPublishRequest{});
  EXPECT_EQ(publish.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(publish.subscription_id, subscription.subscription_id);
  const auto* data =
      std::get_if<OpcUaBinaryDataChangeNotification>(
          &publish.notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].value.value.get<double>(), 12.5);
}

TEST_F(OpcUaBinaryRuntimeTest,
       TransfersSubscriptionsAcrossConnectionsWithBinarySurface) {
  OpcUaBinaryConnectionState source_connection;
  CreateAndActivate(source_connection);

  const auto created_subscription =
      Handle<OpcUaBinaryCreateSubscriptionResponse>(
          source_connection,
          OpcUaBinaryCreateSubscriptionRequest{
              .parameters = {.publishing_interval_ms = 100,
                             .lifetime_count = 60,
                             .max_keep_alive_count = 3,
                             .publishing_enabled = true}});
  ASSERT_EQ(created_subscription.status.code(), scada::StatusCode::Good);

  const auto created_items = Handle<OpcUaBinaryCreateMonitoredItemsResponse>(
      source_connection,
      OpcUaBinaryCreateMonitoredItemsRequest{
          .subscription_id = created_subscription.subscription_id,
          .items_to_create = {{.item_to_monitor =
                                   {.node_id = NumericNode(21),
                                    .attribute_id = scada::AttributeId::Value},
                               .requested_parameters =
                                   {.client_handle = 55,
                                    .sampling_interval_ms = 0,
                                    .queue_size = 1,
                                    .discard_oldest = true}}}});
  ASSERT_EQ(created_items.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(monitored_item_service_.items.size(), 1u);

  monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{77.0}, {}, now_, now_});

  OpcUaBinaryConnectionState target_connection;
  CreateAndActivate(target_connection);
  const auto transferred =
      Handle<OpcUaBinaryTransferSubscriptionsResponse>(
          target_connection,
          OpcUaBinaryTransferSubscriptionsRequest{
              .subscription_ids = {created_subscription.subscription_id},
              .send_initial_values = true});
  EXPECT_EQ(transferred.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  const auto source_publish =
      Handle<OpcUaBinaryPublishResponse>(source_connection,
                                         OpcUaBinaryPublishRequest{});
  EXPECT_EQ(source_publish.status.code(), scada::StatusCode::Bad_NothingToDo);

  now_ = now_ + base::TimeDelta::FromMilliseconds(100);
  const auto target_publish =
      Handle<OpcUaBinaryPublishResponse>(target_connection,
                                         OpcUaBinaryPublishRequest{});
  EXPECT_EQ(target_publish.subscription_id,
            created_subscription.subscription_id);
  const auto* data = std::get_if<OpcUaBinaryDataChangeNotification>(
      &target_publish.notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].value.value.get<double>(), 77.0);
}

TEST_F(OpcUaBinaryRuntimeTest, CloseSessionClearsAttachedState) {
  OpcUaBinaryConnectionState connection;
  const auto [session_id, authentication_token] = CreateAndActivate(connection);

  const auto closed = WaitAwaitable(
      executor_, runtime_->CloseSession(connection,
                                        OpcUaBinaryCloseSessionRequest{
                                            .session_id = session_id,
                                            .authentication_token =
                                                authentication_token,
                                        }));
  EXPECT_EQ(closed.status.code(), scada::StatusCode::Good);
  EXPECT_FALSE(connection.authentication_token.has_value());

  const auto response = Handle<OpcUaBinaryReadResponse>(
      connection,
      OpcUaBinaryReadRequest{
          .inputs = {{.node_id = NumericNode(31),
                      .attribute_id = scada::AttributeId::Value}}});
  EXPECT_EQ(response.status.code(), scada::StatusCode::Bad_SessionIsLoggedOff);
}

TEST_F(OpcUaBinaryRuntimeTest,
       RejectsHistoryReadRawWithoutActivatedSession) {
  OpcUaBinaryConnectionState connection;

  const auto response = Handle<OpcUaBinaryHistoryReadRawResponse>(
      connection,
      OpcUaBinaryHistoryReadRawRequest{
          .details =
              {.node_id = NumericNode(41),
               .from = now_ - base::TimeDelta::FromMinutes(10),
               .to = now_,
               .max_count = 5}});
  EXPECT_EQ(response.result.status.code(),
            scada::StatusCode::Bad_SessionIsLoggedOff);
  EXPECT_TRUE(response.result.values.empty());
  EXPECT_TRUE(response.result.continuation_point.empty());
}

TEST_F(OpcUaBinaryRuntimeTest,
       RejectsHistoryReadEventsWithoutActivatedSession) {
  OpcUaBinaryConnectionState connection;

  const auto response = Handle<OpcUaBinaryHistoryReadEventsResponse>(
      connection,
      OpcUaBinaryHistoryReadEventsRequest{
          .details =
              {.node_id = NumericNode(42),
               .from = now_ - base::TimeDelta::FromMinutes(30),
               .to = now_}});
  EXPECT_EQ(response.result.status.code(),
            scada::StatusCode::Bad_SessionIsLoggedOff);
  EXPECT_TRUE(response.result.events.empty());
}

}  // namespace
}  // namespace opcua
