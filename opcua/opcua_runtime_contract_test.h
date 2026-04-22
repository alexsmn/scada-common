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

}  // namespace opcua::test
