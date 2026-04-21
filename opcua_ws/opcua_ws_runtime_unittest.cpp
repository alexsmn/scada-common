#include "opcua_ws/opcua_ws_runtime.h"

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

#include <future>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace opcua_ws {
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

class OpcUaWsRuntimeTest : public Test {
 protected:
  template <typename T>
  OpcUaWsResponseMessage Handle(OpcUaWsConnectionState& connection,
                                T body,
                                scada::UInt32 request_handle = 1) {
    return WaitAwaitable(executor_,
                         runtime_->Handle(connection,
                                          {.request_handle = request_handle,
                                           .body = std::move(body)}));
  }

  std::pair<scada::NodeId, scada::NodeId> CreateAndActivate(
      OpcUaWsConnectionState& connection,
      std::u16string user_name = u"operator",
      std::u16string password = u"secret") {
    const auto created_message =
        Handle(connection, OpcUaWsCreateSessionRequest{});
    const auto* created =
        std::get_if<OpcUaWsCreateSessionResponse>(&created_message.body);
    EXPECT_NE(created, nullptr);
    EXPECT_EQ(created->status.code(), scada::StatusCode::Good);

    const auto activated_message = Handle(
        connection,
        OpcUaWsActivateSessionRequest{
            .session_id = created->session_id,
            .authentication_token = created->authentication_token,
            .user_name = scada::LocalizedText{std::move(user_name)},
            .password = scada::LocalizedText{std::move(password)},
        },
        2);
    const auto* activated =
        std::get_if<OpcUaWsActivateSessionResponse>(&activated_message.body);
    EXPECT_NE(activated, nullptr);
    EXPECT_EQ(activated->status.code(), scada::StatusCode::Good);
    EXPECT_FALSE(activated->resumed);
    return {created->session_id, created->authentication_token};
  }

  base::Time now_ = ParseTime("2026-04-20 18:00:00");
  const scada::NodeId expected_user_id_ = NumericNode(700, 5);
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  StrictMock<scada::MockAttributeService> attribute_service_;
  StrictMock<scada::MockViewService> view_service_;
  StrictMock<scada::MockHistoryService> history_service_;
  StrictMock<scada::MockMethodService> method_service_;
  StrictMock<scada::MockNodeManagementService> node_management_service_;
  TestMonitoredItemService monitored_item_service_;
  OpcUaWsSessionManager session_manager_{{
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
  std::unique_ptr<OpcUaWsRuntime> runtime_ = std::make_unique<OpcUaWsRuntime>(
      OpcUaWsRuntimeContext{
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

TEST_F(OpcUaWsRuntimeTest, RoutesReadRequestsThroughActivatedSessionUser) {
  OpcUaWsConnectionState connection;
  CreateAndActivate(connection);

  ReadRequest request{
      .inputs = {{.node_id = NumericNode(1),
                  .attribute_id = scada::AttributeId::DisplayName}}};
  EXPECT_CALL(attribute_service_, Read(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
                           const scada::ReadCallback& callback) {
        EXPECT_EQ(context.user_id(), expected_user_id_);
        EXPECT_THAT(*inputs, ElementsAre(request.inputs[0]));
        callback(scada::StatusCode::Good,
                 {scada::DataValue{scada::LocalizedText{u"Pump"}, {},
                                   now_, now_}});
      }));

  const auto response_message = Handle(connection, request, 3);
  const auto* response = std::get_if<ReadResponse>(&response_message.body);
  ASSERT_NE(response, nullptr);
  EXPECT_EQ(response->status.code(), scada::StatusCode::Good);
  ASSERT_EQ(response->results.size(), 1u);
  EXPECT_EQ(response->results[0].value,
            scada::Variant{scada::LocalizedText{u"Pump"}});
}

TEST_F(OpcUaWsRuntimeTest, DetachAndResumePreserveLiveSubscriptionState) {
  OpcUaWsConnectionState first_connection;
  const auto [session_id, authentication_token] =
      CreateAndActivate(first_connection);

  const auto created_subscription = Handle(
      first_connection,
      OpcUaWsCreateSubscriptionRequest{
          .parameters = {.publishing_interval_ms = 100,
                         .lifetime_count = 60,
                         .max_keep_alive_count = 3,
                         .publishing_enabled = true}},
      3);
  const auto* subscription_response =
      std::get_if<OpcUaWsCreateSubscriptionResponse>(&created_subscription.body);
  ASSERT_NE(subscription_response, nullptr);

  const auto created_items = Handle(
      first_connection,
      OpcUaWsCreateMonitoredItemsRequest{
          .subscription_id = subscription_response->subscription_id,
          .items_to_create = {{.item_to_monitor =
                                   {.node_id = NumericNode(11),
                                    .attribute_id = scada::AttributeId::Value},
                               .requested_parameters =
                                   {.client_handle = 44,
                                    .sampling_interval_ms = 0,
                                    .queue_size = 1,
                                    .discard_oldest = true}}}},
      4);
  const auto* create_items_response =
      std::get_if<OpcUaWsCreateMonitoredItemsResponse>(&created_items.body);
  ASSERT_NE(create_items_response, nullptr);
  ASSERT_EQ(monitored_item_service_.items.size(), 1u);

  monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{12.5}, {}, now_, now_});
  runtime_->Detach(first_connection);
  EXPECT_FALSE(first_connection.authentication_token.has_value());

  OpcUaWsConnectionState second_connection;
  const auto resumed_message = Handle(
      second_connection,
      OpcUaWsActivateSessionRequest{
          .session_id = session_id,
          .authentication_token = authentication_token,
      },
      5);
  const auto* resumed =
      std::get_if<OpcUaWsActivateSessionResponse>(&resumed_message.body);
  ASSERT_NE(resumed, nullptr);
  EXPECT_EQ(resumed->status.code(), scada::StatusCode::Good);
  EXPECT_TRUE(resumed->resumed);

  const auto publish_message =
      Handle(second_connection, OpcUaWsPublishRequest{}, 6);
  const auto* publish = std::get_if<OpcUaWsPublishResponse>(&publish_message.body);
  ASSERT_NE(publish, nullptr);
  EXPECT_EQ(publish->status.code(), scada::StatusCode::Good);
  EXPECT_EQ(publish->subscription_id, subscription_response->subscription_id);
  const auto* data =
      std::get_if<OpcUaWsDataChangeNotification>(
          &publish->notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].value.value.get<double>(), 12.5);
}

TEST_F(OpcUaWsRuntimeTest,
       TransferSubscriptionsUsesGlobalOwnershipAcrossSessions) {
  OpcUaWsConnectionState first_connection;
  CreateAndActivate(first_connection);

  const auto created_subscription = Handle(
      first_connection,
      OpcUaWsCreateSubscriptionRequest{
          .parameters = {.publishing_interval_ms = 100,
                         .lifetime_count = 60,
                         .max_keep_alive_count = 3,
                         .publishing_enabled = true}},
      3);
  const auto* subscription_response =
      std::get_if<OpcUaWsCreateSubscriptionResponse>(&created_subscription.body);
  ASSERT_NE(subscription_response, nullptr);

  const auto created_items = Handle(
      first_connection,
      OpcUaWsCreateMonitoredItemsRequest{
          .subscription_id = subscription_response->subscription_id,
          .items_to_create = {{.item_to_monitor =
                                   {.node_id = NumericNode(21),
                                    .attribute_id = scada::AttributeId::Value},
                               .requested_parameters =
                                   {.client_handle = 55,
                                    .sampling_interval_ms = 0,
                                    .queue_size = 1,
                                    .discard_oldest = true}}}},
      4);
  ASSERT_NE(std::get_if<OpcUaWsCreateMonitoredItemsResponse>(&created_items.body),
            nullptr);
  ASSERT_EQ(monitored_item_service_.items.size(), 1u);
  monitored_item_service_.items[0]->NotifyDataChange(
      scada::DataValue{scada::Variant{77.0}, {}, now_, now_});

  OpcUaWsConnectionState second_connection;
  CreateAndActivate(second_connection);
  const auto transfer_message = Handle(
      second_connection,
      OpcUaWsTransferSubscriptionsRequest{
          .subscription_ids = {subscription_response->subscription_id},
          .send_initial_values = true},
      5);
  const auto* transfer =
      std::get_if<OpcUaWsTransferSubscriptionsResponse>(&transfer_message.body);
  ASSERT_NE(transfer, nullptr);
  EXPECT_EQ(transfer->results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  const auto source_publish =
      Handle(first_connection, OpcUaWsPublishRequest{}, 6);
  const auto* source_response =
      std::get_if<OpcUaWsPublishResponse>(&source_publish.body);
  ASSERT_NE(source_response, nullptr);
  EXPECT_EQ(source_response->status.code(), scada::StatusCode::Bad_NothingToDo);

  const auto target_publish =
      Handle(second_connection, OpcUaWsPublishRequest{}, 7);
  const auto* target_response =
      std::get_if<OpcUaWsPublishResponse>(&target_publish.body);
  ASSERT_NE(target_response, nullptr);
  EXPECT_EQ(target_response->subscription_id,
            subscription_response->subscription_id);
  const auto* data =
      std::get_if<OpcUaWsDataChangeNotification>(
          &target_response->notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].value.value.get<double>(), 77.0);
}

TEST_F(OpcUaWsRuntimeTest, CloseSessionRemovesAttachedRuntimeState) {
  OpcUaWsConnectionState connection;
  const auto [session_id, authentication_token] = CreateAndActivate(connection);

  const auto closed_message = Handle(
      connection,
      OpcUaWsCloseSessionRequest{
          .session_id = session_id,
          .authentication_token = authentication_token,
      },
      3);
  const auto* closed =
      std::get_if<OpcUaWsCloseSessionResponse>(&closed_message.body);
  ASSERT_NE(closed, nullptr);
  EXPECT_EQ(closed->status.code(), scada::StatusCode::Good);
  EXPECT_FALSE(connection.authentication_token.has_value());

  const auto read_message = Handle(
      connection,
      ReadRequest{.inputs = {{.node_id = NumericNode(31),
                              .attribute_id = scada::AttributeId::Value}}},
      4);
  const auto* fault = std::get_if<OpcUaWsServiceFault>(&read_message.body);
  ASSERT_NE(fault, nullptr);
  EXPECT_EQ(fault->status.code(), scada::StatusCode::Bad_SessionIsLoggedOff);
}

TEST_F(OpcUaWsRuntimeTest, BrowseAndBrowseNextUseSessionScopedContinuationPoints) {
  OpcUaWsConnectionState connection;
  CreateAndActivate(connection);

  EXPECT_CALL(view_service_, Browse(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::vector<scada::BrowseDescription>& inputs,
                           const scada::BrowseCallback& callback) {
        EXPECT_EQ(context.user_id(), expected_user_id_);
        ASSERT_EQ(inputs.size(), 1u);
        callback(scada::StatusCode::Good,
                 {scada::BrowseResult{
                     .status_code = scada::StatusCode::Good,
                     .references = {{.reference_type_id = NumericNode(901),
                                     .forward = true,
                                     .node_id = NumericNode(902)},
                                    {.reference_type_id = NumericNode(903),
                                     .forward = false,
                                     .node_id = NumericNode(904)},
                                    {.reference_type_id = NumericNode(905),
                                     .forward = true,
                                     .node_id = NumericNode(906)}}}});
      }));

  const auto browse_message = Handle(
      connection,
      BrowseRequest{
          .requested_max_references_per_node = 2,
          .inputs = {{.node_id = NumericNode(900),
                      .direction = scada::BrowseDirection::Both,
                      .reference_type_id = NumericNode(910),
                      .include_subtypes = true}}},
      3);
  const auto* browse = std::get_if<BrowseResponse>(&browse_message.body);
  ASSERT_NE(browse, nullptr);
  ASSERT_EQ(browse->results.size(), 1u);
  ASSERT_EQ(browse->results[0].references.size(), 2u);
  ASSERT_FALSE(browse->results[0].continuation_point.empty());
  EXPECT_EQ(browse->results[0].references[0].node_id, NumericNode(902));
  EXPECT_EQ(browse->results[0].references[1].node_id, NumericNode(904));

  const auto browse_next_message = Handle(
      connection,
      BrowseNextRequest{
          .continuation_points = {browse->results[0].continuation_point}},
      4);
  const auto* browse_next =
      std::get_if<BrowseNextResponse>(&browse_next_message.body);
  ASSERT_NE(browse_next, nullptr);
  ASSERT_EQ(browse_next->results.size(), 1u);
  EXPECT_EQ(browse_next->results[0].status_code, scada::StatusCode::Good);
  ASSERT_EQ(browse_next->results[0].references.size(), 1u);
  EXPECT_EQ(browse_next->results[0].references[0].node_id, NumericNode(906));

  const auto invalid_message = Handle(
      connection,
      BrowseNextRequest{
          .continuation_points = {browse->results[0].continuation_point}},
      5);
  const auto* invalid = std::get_if<BrowseNextResponse>(&invalid_message.body);
  ASSERT_NE(invalid, nullptr);
  ASSERT_EQ(invalid->results.size(), 1u);
  EXPECT_EQ(invalid->results[0].status_code, scada::StatusCode::Bad_WrongIndex);
}

TEST_F(OpcUaWsRuntimeTest, PublishRequestWaitsForKeepAliveDeadline) {
  OpcUaWsConnectionState connection;
  CreateAndActivate(connection);

  const auto created_subscription = Handle(
      connection,
      OpcUaWsCreateSubscriptionRequest{
          .parameters = {.publishing_interval_ms = 100,
                         .lifetime_count = 60,
                         .max_keep_alive_count = 3,
                         .publishing_enabled = true}},
      3);
  ASSERT_NE(
      std::get_if<OpcUaWsCreateSubscriptionResponse>(&created_subscription.body),
      nullptr);

  promise<OpcUaWsResponseMessage> publish_promise;
  CoSpawn(MakeTestAnyExecutor(executor_),
          [this, &connection, &publish_promise]() mutable -> Awaitable<void> {
            try {
              publish_promise.resolve(
                  co_await runtime_->Handle(connection,
                                            {.request_handle = 4,
                                             .body = OpcUaWsPublishRequest{}}));
            } catch (...) {
              publish_promise.reject(std::current_exception());
            }
          });

  const auto drain_ready = [&] {
    for (size_t i = 0; i < 8; ++i)
      executor_->Poll();
  };

  drain_ready();
  EXPECT_EQ(publish_promise.wait_for(0ms), promise_wait_status::timeout);

  now_ = now_ + base::TimeDelta::FromMilliseconds(299);
  executor_->Advance(299ms);
  drain_ready();
  EXPECT_EQ(publish_promise.wait_for(0ms), promise_wait_status::timeout);

  now_ = now_ + base::TimeDelta::FromMilliseconds(1);
  executor_->Advance(1ms);
  drain_ready();
  ASSERT_NE(publish_promise.wait_for(0ms), promise_wait_status::timeout);

  const auto publish_message = publish_promise.get();
  const auto* publish = std::get_if<OpcUaWsPublishResponse>(&publish_message.body);
  ASSERT_NE(publish, nullptr);
  EXPECT_EQ(publish->status.code(), scada::StatusCode::Good);
  EXPECT_EQ(publish->notification_message.sequence_number, 0u);
  EXPECT_TRUE(publish->notification_message.notification_data.empty());
}

}  // namespace
}  // namespace opcua_ws
