#include "opcua_ws/opcua_ws_subscription.h"

#include "base/time_utils.h"
#include "scada/monitoring_parameters.h"
#include "scada/test/test_monitored_item.h"

#include <boost/json/parse.hpp>
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
    if (return_null_for_all_requests) {
      return nullptr;
    }
    auto item = std::make_shared<scada::TestMonitoredItem>();
    items.push_back(item);
    return item;
  }

  bool return_null_for_all_requests = false;
  std::vector<scada::ReadValueId> created_value_ids;
  std::vector<scada::MonitoringParameters> created_params;
  std::vector<std::shared_ptr<scada::TestMonitoredItem>> items;
};

TEST(OpcUaWsSubscriptionTest, PublishesDataChangesAcknowledgesAndRepublishes) {
  TestMonitoredItemService monitored_item_service;
  const auto start = ParseTime("2026-04-20 10:00:00");
  OpcUaWsSubscription subscription{
      17,
      {.publishing_interval_ms = 100,
       .lifetime_count = 60,
       .max_keep_alive_count = 3,
       .max_notifications_per_publish = 0,
       .publishing_enabled = true,
       .priority = 0},
      monitored_item_service,
      start};

  const auto create = subscription.CreateMonitoredItems(
      {.subscription_id = 17,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(101),
                                 .attribute_id = scada::AttributeId::Value},
                            .requested_parameters =
                                {.client_handle = 44,
                                 .sampling_interval_ms = 250,
                                 .queue_size = 2,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(create.results.size(), 1u);
  EXPECT_EQ(create.results[0].status.code(), scada::StatusCode::Good);
  ASSERT_EQ(monitored_item_service.items.size(), 1u);

  monitored_item_service.items[0]->NotifyDataChange(scada::DataValue{
      scada::Variant{12.5}, {}, start,
      ParseTime("2026-04-20 10:00:01")});
  EXPECT_FALSE(subscription.TryPublish(
                   start + base::TimeDelta::FromMilliseconds(99))
                   .has_value());
  const auto first_publish =
      subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(100));
  ASSERT_TRUE(first_publish.has_value());
  EXPECT_EQ(first_publish->subscription_id, 17u);
  EXPECT_EQ(first_publish->available_sequence_numbers,
            (std::vector<scada::UInt32>{1u}));
  ASSERT_EQ(first_publish->notification_message.notification_data.size(), 1u);
  const auto* first_data =
      std::get_if<OpcUaWsDataChangeNotification>(
          &first_publish->notification_message.notification_data[0]);
  ASSERT_NE(first_data, nullptr);
  ASSERT_EQ(first_data->monitored_items.size(), 1u);
  EXPECT_EQ(first_data->monitored_items[0].client_handle, 44u);
  EXPECT_EQ(first_data->monitored_items[0].value.value.get<double>(), 12.5);

  monitored_item_service.items[0]->NotifyDataChange(scada::DataValue{
      scada::Variant{13.5}, {},
      start + base::TimeDelta::FromMilliseconds(300),
      ParseTime("2026-04-20 10:00:04")});
  EXPECT_EQ(subscription.Acknowledge(std::vector<scada::UInt32>{1}),
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));
  const auto second_publish =
      subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(300));
  ASSERT_TRUE(second_publish.has_value());
  EXPECT_TRUE(second_publish->results.empty());
  EXPECT_EQ(second_publish->available_sequence_numbers,
            (std::vector<scada::UInt32>{2u}));

  const auto republish_first = subscription.Republish(1);
  EXPECT_EQ(republish_first.status.code(), scada::StatusCode::Bad);
  const auto republish_second = subscription.Republish(2);
  EXPECT_EQ(republish_second.status.code(), scada::StatusCode::Good);
  EXPECT_EQ(republish_second.notification_message.sequence_number, 2u);
}

TEST(OpcUaWsSubscriptionTest, GeneratesKeepAliveAndQueuesWhilePublishingDisabled) {
  TestMonitoredItemService monitored_item_service;
  const auto start = ParseTime("2026-04-20 11:00:00");
  OpcUaWsSubscription subscription{
      19,
      {.publishing_interval_ms = 100,
       .lifetime_count = 60,
       .max_keep_alive_count = 3,
       .max_notifications_per_publish = 0,
       .publishing_enabled = true,
       .priority = 0},
      monitored_item_service,
      start};

  ASSERT_FALSE(subscription.TryPublish(start).has_value());
  const auto keep_alive = subscription.TryPublish(
      start + base::TimeDelta::FromMilliseconds(100));
  ASSERT_TRUE(keep_alive.has_value());
  EXPECT_EQ(keep_alive->notification_message.sequence_number, 1u);
  EXPECT_TRUE(keep_alive->notification_message.notification_data.empty());

  const auto create = subscription.CreateMonitoredItems(
      {.subscription_id = 19,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(201),
                                 .attribute_id = scada::AttributeId::Value},
                            .requested_parameters = {.client_handle = 88,
                                                     .sampling_interval_ms = 0,
                                                     .queue_size = 1,
                                                     .discard_oldest = true}}}});
  ASSERT_EQ(create.results.size(), 1u);
  subscription.SetPublishingEnabled(false);
  monitored_item_service.items[0]->NotifyDataChange(scada::DataValue{
      scada::Variant{77.0}, {}, start + base::TimeDelta::FromSeconds(1),
      start + base::TimeDelta::FromSeconds(1)});
  const auto disabled_keep_alive =
      subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(1050));
  ASSERT_TRUE(disabled_keep_alive.has_value());
  EXPECT_EQ(disabled_keep_alive->notification_message.sequence_number, 1u);
  EXPECT_TRUE(disabled_keep_alive->notification_message.notification_data.empty());

  subscription.SetPublishingEnabled(true);
  EXPECT_FALSE(subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(1060))
                   .has_value());
  const auto publish =
      subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(1150));
  ASSERT_TRUE(publish.has_value());
  const auto* data =
      std::get_if<OpcUaWsDataChangeNotification>(
          &publish->notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].value.value.get<double>(), 77.0);
}

TEST(OpcUaWsSubscriptionTest,
     WaitsForPublishingIntervalBeforeSendingDataOrKeepAlive) {
  TestMonitoredItemService monitored_item_service;
  const auto start = ParseTime("2026-04-20 11:30:00");
  OpcUaWsSubscription subscription{
      37,
      {.publishing_interval_ms = 100,
       .lifetime_count = 60,
       .max_keep_alive_count = 3,
       .max_notifications_per_publish = 0,
       .publishing_enabled = true,
       .priority = 0},
      monitored_item_service,
      start};

  const auto create = subscription.CreateMonitoredItems(
      {.subscription_id = 37,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(3701),
                                 .attribute_id = scada::AttributeId::Value},
                            .requested_parameters =
                                {.client_handle = 37,
                                 .sampling_interval_ms = 0,
                                 .queue_size = 1,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(create.results.size(), 1u);
  ASSERT_EQ(monitored_item_service.items.size(), 1u);

  EXPECT_FALSE(subscription.TryPublish(start).has_value());

  monitored_item_service.items[0]->NotifyDataChange(scada::DataValue{
      scada::Variant{37.0}, {}, start, start});
  EXPECT_FALSE(subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(99))
                   .has_value());

  const auto data_publish =
      subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(100));
  ASSERT_TRUE(data_publish.has_value());
  const auto* data =
      std::get_if<OpcUaWsDataChangeNotification>(
          &data_publish->notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].value.value.get<double>(), 37.0);

  EXPECT_FALSE(subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(399))
                   .has_value());
  const auto keep_alive =
      subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(400));
  ASSERT_TRUE(keep_alive.has_value());
  EXPECT_EQ(keep_alive->notification_message.sequence_number, 2u);
  EXPECT_TRUE(keep_alive->notification_message.notification_data.empty());
}

TEST(OpcUaWsSubscriptionTest,
     ProjectsDefaultEventFieldsAndDropsOldQueuedEventsByQueueSize) {
  TestMonitoredItemService monitored_item_service;
  const auto start = ParseTime("2026-04-20 12:00:00");
  OpcUaWsSubscription subscription{
      23,
      {.publishing_interval_ms = 100,
       .lifetime_count = 60,
       .max_keep_alive_count = 5,
       .max_notifications_per_publish = 0,
       .publishing_enabled = true,
       .priority = 0},
      monitored_item_service,
      start};

  const boost::json::value event_filter = boost::json::parse(R"({
    "Type":"EventFilter",
    "Body":{"SelectClauses":[
      {"BrowsePath":[{"Name":"EventId"}]},
      {"BrowsePath":[{"Name":"Message"}]},
      {"BrowsePath":[{"Name":"Severity"}]}
    ]}}
  )");
  const auto create = subscription.CreateMonitoredItems(
      {.subscription_id = 23,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(2253, 0),
                                 .attribute_id =
                                     static_cast<scada::AttributeId>(12)},
                            .requested_parameters =
                                {.client_handle = 5,
                                 .sampling_interval_ms = 0,
                                 .filter = OpcUaWsMonitoringFilter{event_filter},
                                 .queue_size = 1,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(create.results.size(), 1u);

  scada::Event first_event;
  first_event.event_id = 11;
  first_event.event_type_id = NumericNode(2041, 0);
  first_event.node_id = NumericNode(3001);
  first_event.time = start;
  first_event.receive_time = ParseTime("2026-04-20 12:00:01");
  first_event.message = u"first";
  first_event.severity = 400;

  scada::Event second_event = first_event;
  second_event.event_id = 22;
  second_event.message = u"second";
  second_event.severity = 700;

  monitored_item_service.items[0]->NotifyEvent(first_event);
  monitored_item_service.items[0]->NotifyEvent(second_event);

  const auto publish =
      subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(100));
  ASSERT_TRUE(publish.has_value());
  const auto* events =
      std::get_if<OpcUaWsEventNotificationList>(
          &publish->notification_message.notification_data[0]);
  ASSERT_NE(events, nullptr);
  ASSERT_EQ(events->events.size(), 1u);
  EXPECT_EQ(events->events[0].client_handle, 5u);
  ASSERT_EQ(events->events[0].event_fields.size(), 3u);
  EXPECT_EQ(events->events[0].event_fields[0].get<scada::UInt64>(), 22u);
  EXPECT_EQ(events->events[0].event_fields[1].get<scada::LocalizedText>(),
            scada::LocalizedText{u"second"});
  EXPECT_EQ(events->events[0].event_fields[2].get<scada::UInt32>(), 700u);
}

TEST(OpcUaWsSubscriptionTest,
     RebindsModifiedItemsAndIgnoresLateCallbacksFromPreviousBinding) {
  TestMonitoredItemService monitored_item_service;
  const auto start = ParseTime("2026-04-20 13:00:00");
  OpcUaWsSubscription subscription{
      29,
      {.publishing_interval_ms = 100,
       .lifetime_count = 60,
       .max_keep_alive_count = 5,
       .max_notifications_per_publish = 0,
       .publishing_enabled = true,
       .priority = 0},
      monitored_item_service,
      start};

  const auto create = subscription.CreateMonitoredItems(
      {.subscription_id = 29,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(401),
                                 .attribute_id = scada::AttributeId::Value},
                            .requested_parameters =
                                {.client_handle = 9,
                                 .sampling_interval_ms = 0,
                                 .queue_size = 2,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(create.results.size(), 1u);
  ASSERT_EQ(monitored_item_service.items.size(), 1u);
  const auto old_item = monitored_item_service.items[0];

  const auto modify = subscription.ModifyMonitoredItems(
      {.subscription_id = 29,
       .items_to_modify = {{.monitored_item_id = create.results[0].monitored_item_id,
                            .requested_parameters =
                                {.client_handle = 99,
                                 .sampling_interval_ms = 50,
                                 .queue_size = 2,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(modify.results.size(), 1u);
  ASSERT_EQ(monitored_item_service.items.size(), 2u);
  const auto new_item = monitored_item_service.items[1];

  old_item->NotifyDataChange(scada::DataValue{
      scada::Variant{1.0}, {}, start, start});
  EXPECT_FALSE(subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(50))
                   .has_value());

  new_item->NotifyDataChange(scada::DataValue{
      scada::Variant{2.0}, {}, start + base::TimeDelta::FromMilliseconds(200),
      start + base::TimeDelta::FromMilliseconds(200)});
  const auto publish =
      subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(100));
  ASSERT_TRUE(publish.has_value());
  const auto* data =
      std::get_if<OpcUaWsDataChangeNotification>(
          &publish->notification_message.notification_data[0]);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(data->monitored_items[0].client_handle, 99u);

  const auto deleted = subscription.DeleteMonitoredItems(
      {.subscription_id = 29,
       .monitored_item_ids = {create.results[0].monitored_item_id}});
  EXPECT_EQ(deleted.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));
  new_item->NotifyDataChange(scada::DataValue{
      scada::Variant{3.0}, {}, start + base::TimeDelta::FromMilliseconds(400),
      start + base::TimeDelta::FromMilliseconds(400)});
  EXPECT_FALSE(subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(200))
                   .has_value());
}

TEST(OpcUaWsSubscriptionTest,
     CreateMonitoredItemsReportsPreciseStatusForUnknownNodeAndBadAttribute) {
  TestMonitoredItemService monitored_item_service;
  monitored_item_service.return_null_for_all_requests = true;
  const auto start = ParseTime("2026-04-20 14:00:00");

  OpcUaWsSubscription subscription{
      31,
      {.publishing_interval_ms = 100,
       .lifetime_count = 60,
       .max_keep_alive_count = 3,
       .max_notifications_per_publish = 0,
       .publishing_enabled = true,
       .priority = 0},
      monitored_item_service,
      start};

  const auto unknown_node = subscription.CreateMonitoredItems(
      {.subscription_id = 31,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(999),
                                 .attribute_id = scada::AttributeId::Value},
                            .requested_parameters =
                                {.client_handle = 1,
                                 .sampling_interval_ms = 250,
                                 .queue_size = 1,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(unknown_node.results.size(), 1u);
  EXPECT_EQ(unknown_node.results[0].status.code(),
            scada::StatusCode::Bad_WrongNodeId);

  const auto bad_attribute = subscription.CreateMonitoredItems(
      {.subscription_id = 31,
       .items_to_create = {{.item_to_monitor =
                                {.node_id = NumericNode(999),
                                 .attribute_id = scada::AttributeId::Description},
                            .requested_parameters =
                                {.client_handle = 2,
                                 .sampling_interval_ms = 250,
                                 .queue_size = 1,
                                 .discard_oldest = true}}}});
  ASSERT_EQ(bad_attribute.results.size(), 1u);
  EXPECT_EQ(bad_attribute.results[0].status.code(),
            scada::StatusCode::Bad_WrongAttributeId);
}

}  // namespace
}  // namespace opcua_ws
