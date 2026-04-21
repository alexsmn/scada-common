#include "opcua_ws/opcua_ws_session.h"

#include "base/time_utils.h"
#include "scada/monitoring_parameters.h"
#include "scada/test/test_monitored_item.h"

#include <gtest/gtest.h>

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

OpcUaWsSession MakeSession(scada::MonitoredItemService& monitored_item_service,
                           const std::function<base::Time()>& now) {
  return OpcUaWsSession{{
      .session_id = NumericNode(1001),
      .authentication_token = NumericNode(2001, 3),
      .service_context = scada::ServiceContext{}.with_user_id(NumericNode(77, 4)),
      .monitored_item_service = monitored_item_service,
      .now = now,
  }};
}

TEST(OpcUaWsSessionTest, PublishesAcrossSubscriptionsRoundRobinAndAcknowledges) {
  TestMonitoredItemService monitored_item_service;
  auto now = ParseTime("2026-04-20 14:00:00");
  auto session = MakeSession(monitored_item_service, [&] { return now; });

  const auto first_subscription = session.CreateSubscription(
      {.parameters = {.publishing_interval_ms = 100,
                      .lifetime_count = 60,
                      .max_keep_alive_count = 3,
                      .publishing_enabled = true}});
  const auto second_subscription = session.CreateSubscription(
      {.parameters = {.publishing_interval_ms = 100,
                      .lifetime_count = 60,
                      .max_keep_alive_count = 3,
                      .publishing_enabled = true}});

  const auto first_items = session.CreateMonitoredItems(
      {.subscription_id = first_subscription.subscription_id,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(1),
                                 .attribute_id = scada::AttributeId::Value},
                            .requested_parameters =
                                {.client_handle = 11,
                                 .sampling_interval_ms = 0,
                                 .queue_size = 2,
                                 .discard_oldest = true}}}});
  const auto second_items = session.CreateMonitoredItems(
      {.subscription_id = second_subscription.subscription_id,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(2),
                                 .attribute_id = scada::AttributeId::Value},
                            .requested_parameters =
                                {.client_handle = 22,
                                 .sampling_interval_ms = 0,
                                 .queue_size = 2,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(first_items.results.size(), 1u);
  ASSERT_EQ(second_items.results.size(), 1u);
  ASSERT_EQ(monitored_item_service.items.size(), 2u);

  monitored_item_service.items[0]->NotifyDataChange(scada::DataValue{
      scada::Variant{10.0}, {}, now, now});
  monitored_item_service.items[1]->NotifyDataChange(scada::DataValue{
      scada::Variant{20.0}, {}, now, now});

  now = now + base::TimeDelta::FromMilliseconds(100);
  const auto first_publish = session.Publish({});
  EXPECT_EQ(first_publish.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(first_publish.subscription_id, first_subscription.subscription_id);
  const auto* first_data = std::get_if<OpcUaWsDataChangeNotification>(
      &first_publish.notification_message.notification_data[0]);
  ASSERT_NE(first_data, nullptr);
  EXPECT_EQ(first_data->monitored_items[0].client_handle, 11u);

  now = now + base::TimeDelta::FromMilliseconds(100);
  const auto second_publish = session.Publish(
      {.subscription_acknowledgements =
           {{.subscription_id = first_subscription.subscription_id,
             .sequence_number = 1}}});
  EXPECT_EQ(second_publish.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(second_publish.subscription_id, second_subscription.subscription_id);
  EXPECT_EQ(second_publish.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));
  const auto* second_data = std::get_if<OpcUaWsDataChangeNotification>(
      &second_publish.notification_message.notification_data[0]);
  ASSERT_NE(second_data, nullptr);
  EXPECT_EQ(second_data->monitored_items[0].client_handle, 22u);

  const auto republish = session.Republish(
      {.subscription_id = second_subscription.subscription_id,
       .retransmit_sequence_number = 1});
  EXPECT_EQ(republish.status.code(), scada::StatusCode::Good);
}

TEST(OpcUaWsSessionTest, PrimesKeepAliveAndHonorsPublishingMode) {
  TestMonitoredItemService monitored_item_service;
  auto now = ParseTime("2026-04-20 15:00:00");
  auto session = MakeSession(monitored_item_service, [&] { return now; });

  const auto created = session.CreateSubscription(
      {.parameters = {.publishing_interval_ms = 100,
                      .lifetime_count = 60,
                      .max_keep_alive_count = 2,
                      .publishing_enabled = true}});

  const auto first_publish = session.Publish({});
  EXPECT_EQ(first_publish.status.code(), scada::StatusCode::Good);
  EXPECT_TRUE(first_publish.notification_message.notification_data.empty());
  EXPECT_EQ(first_publish.subscription_id, 0u);

  now = now + base::TimeDelta::FromMilliseconds(100);
  const auto keep_alive = session.Publish({});
  EXPECT_EQ(keep_alive.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(keep_alive.subscription_id, created.subscription_id);
  EXPECT_EQ(keep_alive.notification_message.sequence_number, 1u);

  const auto mode = session.SetPublishingMode(
      {.publishing_enabled = false, .subscription_ids = {created.subscription_id}});
  EXPECT_EQ(mode.results, (std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  now = now + base::TimeDelta::FromMilliseconds(300);
  const auto disabled_publish = session.Publish({});
  EXPECT_EQ(disabled_publish.status.code(), scada::StatusCode::Good);
  EXPECT_TRUE(disabled_publish.notification_message.notification_data.empty());
  EXPECT_EQ(disabled_publish.subscription_id, created.subscription_id);
  EXPECT_EQ(disabled_publish.notification_message.sequence_number, 1u);
}

TEST(OpcUaWsSessionTest,
     PollPublishRechecksWithinPublishingIntervalBeforeKeepAliveDeadline) {
  TestMonitoredItemService monitored_item_service;
  auto now = ParseTime("2026-04-20 15:30:00");
  auto session = MakeSession(monitored_item_service, [&] { return now; });

  session.CreateSubscription(
      {.parameters = {.publishing_interval_ms = 100,
                      .lifetime_count = 60,
                      .max_keep_alive_count = 10,
                      .publishing_enabled = true}});

  const auto poll = session.PollPublish();
  ASSERT_FALSE(poll.response.has_value());
  ASSERT_TRUE(poll.wait_for.has_value());
  EXPECT_EQ(*poll.wait_for, base::TimeDelta::FromMilliseconds(100));
}

TEST(OpcUaWsSessionTest, RoutesMonitoredItemOperationsToSubscription) {
  TestMonitoredItemService monitored_item_service;
  auto now = ParseTime("2026-04-20 16:00:00");
  auto session = MakeSession(monitored_item_service, [&] { return now; });

  const auto subscription = session.CreateSubscription(
      {.parameters = {.publishing_interval_ms = 100,
                      .lifetime_count = 60,
                      .max_keep_alive_count = 3,
                      .publishing_enabled = true}});

  const auto created = session.CreateMonitoredItems(
      {.subscription_id = subscription.subscription_id,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(301),
                                 .attribute_id = scada::AttributeId::Value},
                            .requested_parameters =
                                {.client_handle = 7,
                                 .sampling_interval_ms = 0,
                                 .queue_size = 1,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(created.results.size(), 1u);
  const auto monitored_item_id = created.results[0].monitored_item_id;

  const auto modified = session.ModifyMonitoredItems(
      {.subscription_id = subscription.subscription_id,
       .items_to_modify = {{.monitored_item_id = monitored_item_id,
                            .requested_parameters =
                                {.client_handle = 8,
                                 .sampling_interval_ms = 50,
                                 .queue_size = 2,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(modified.results.size(), 1u);
  EXPECT_EQ(modified.results[0].status.code(), scada::StatusCode::Good);

  const auto monitoring_mode = session.SetMonitoringMode(
      {.subscription_id = subscription.subscription_id,
       .monitoring_mode = OpcUaWsMonitoringMode::Sampling,
       .monitored_item_ids = {monitored_item_id}});
  EXPECT_EQ(monitoring_mode.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  const auto deleted_items = session.DeleteMonitoredItems(
      {.subscription_id = subscription.subscription_id,
       .monitored_item_ids = {monitored_item_id}});
  EXPECT_EQ(deleted_items.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  const auto deleted_subscriptions = session.DeleteSubscriptions(
      {.subscription_ids = {subscription.subscription_id}});
  EXPECT_EQ(deleted_subscriptions.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));
}

TEST(OpcUaWsSessionTest, TransfersSubscriptionsBetweenSessions) {
  TestMonitoredItemService monitored_item_service;
  auto now = ParseTime("2026-04-20 17:00:00");
  auto source = MakeSession(monitored_item_service, [&] { return now; });
  auto target = MakeSession(monitored_item_service, [&] { return now; });

  const auto created = source.CreateSubscription(
      {.parameters = {.publishing_interval_ms = 100,
                      .lifetime_count = 60,
                      .max_keep_alive_count = 3,
                      .publishing_enabled = true}});
  const auto items = source.CreateMonitoredItems(
      {.subscription_id = created.subscription_id,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(401),
                                 .attribute_id = scada::AttributeId::Value},
                            .requested_parameters =
                                {.client_handle = 41,
                                 .sampling_interval_ms = 0,
                                 .queue_size = 1,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(items.results.size(), 1u);
  ASSERT_EQ(monitored_item_service.items.size(), 1u);

  monitored_item_service.items[0]->NotifyDataChange(scada::DataValue{
      scada::Variant{55.0}, {}, now, now});

  const auto transferred = target.TransferSubscriptionsFrom(
      source,
      {.subscription_ids = {created.subscription_id}, .send_initial_values = true});
  EXPECT_EQ(transferred.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  const auto source_publish = source.Publish({});
  EXPECT_EQ(source_publish.status.code(), scada::StatusCode::Bad_NothingToDo);

  const auto target_publish = target.Publish({});
  EXPECT_EQ(target_publish.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(target_publish.subscription_id, created.subscription_id);
  const auto* data = std::get_if<OpcUaWsDataChangeNotification>(
      &target_publish.notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].value.value.get<double>(), 55.0);
}

TEST(OpcUaWsSessionTest, StoresBrowseContinuationPointsAndResumesPages) {
  TestMonitoredItemService monitored_item_service;
  auto now = ParseTime("2026-04-20 17:30:00");
  auto session = MakeSession(monitored_item_service, [&] { return now; });

  auto paged = session.StoreBrowseResults(
      {.status = scada::StatusCode::Good,
       .results = {{.status_code = scada::StatusCode::Good,
                    .references = {{.reference_type_id = NumericNode(501),
                                    .forward = true,
                                    .node_id = NumericNode(601)},
                                   {.reference_type_id = NumericNode(502),
                                    .forward = true,
                                    .node_id = NumericNode(602)},
                                   {.reference_type_id = NumericNode(503),
                                    .forward = false,
                                    .node_id = NumericNode(603)}}}}},
      2);

  ASSERT_EQ(paged.results.size(), 1u);
  ASSERT_EQ(paged.results[0].references.size(), 2u);
  ASSERT_FALSE(paged.results[0].continuation_point.empty());
  EXPECT_EQ(paged.results[0].references[0].node_id, NumericNode(601));
  EXPECT_EQ(paged.results[0].references[1].node_id, NumericNode(602));

  const auto next = session.BrowseNext(
      {.continuation_points = {paged.results[0].continuation_point}});
  EXPECT_EQ(next.status.code(), scada::StatusCode::Good);
  ASSERT_EQ(next.results.size(), 1u);
  EXPECT_EQ(next.results[0].status_code, scada::StatusCode::Good);
  ASSERT_EQ(next.results[0].references.size(), 1u);
  EXPECT_TRUE(next.results[0].continuation_point.empty());
  EXPECT_EQ(next.results[0].references[0].node_id, NumericNode(603));

  const auto invalid = session.BrowseNext(
      {.continuation_points = {paged.results[0].continuation_point}});
  ASSERT_EQ(invalid.results.size(), 1u);
  EXPECT_EQ(invalid.results[0].status_code, scada::StatusCode::Bad_WrongIndex);
}

TEST(OpcUaWsSessionTest, ReleasesBrowseContinuationPointsWithoutReturningData) {
  TestMonitoredItemService monitored_item_service;
  auto now = ParseTime("2026-04-20 17:40:00");
  auto session = MakeSession(monitored_item_service, [&] { return now; });

  auto paged = session.StoreBrowseResults(
      {.status = scada::StatusCode::Good,
       .results = {{.status_code = scada::StatusCode::Good,
                    .references = {{.reference_type_id = NumericNode(701),
                                    .forward = true,
                                    .node_id = NumericNode(801)},
                                   {.reference_type_id = NumericNode(702),
                                    .forward = true,
                                    .node_id = NumericNode(802)}}}}},
      1);

  const auto released = session.BrowseNext(
      {.release_continuation_points = true,
       .continuation_points = {paged.results[0].continuation_point}});
  ASSERT_EQ(released.results.size(), 1u);
  EXPECT_EQ(released.results[0].status_code, scada::StatusCode::Good);
  EXPECT_TRUE(released.results[0].references.empty());

  const auto invalid = session.BrowseNext(
      {.continuation_points = {paged.results[0].continuation_point}});
  ASSERT_EQ(invalid.results.size(), 1u);
  EXPECT_EQ(invalid.results[0].status_code, scada::StatusCode::Bad_WrongIndex);
}

}  // namespace
}  // namespace opcua_ws
