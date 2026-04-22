#include "opcua/opcua_server_subscription.h"

#include "base/time_utils.h"
#include "scada/monitoring_parameters.h"
#include "scada/test/test_monitored_item.h"

#include <gtest/gtest.h>

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

TEST(OpcUaServerSubscriptionTest, PublishesAcknowledgesAndRepublishesData) {
  TestMonitoredItemService monitored_item_service;
  const auto start = ParseTime("2026-04-20 10:00:00");
  OpcUaSubscription subscription{
      17,
      {.publishing_interval_ms = 100,
       .lifetime_count = 60,
       .max_keep_alive_count = 3,
       .publishing_enabled = true},
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
  ASSERT_EQ(monitored_item_service.items.size(), 1u);

  monitored_item_service.items[0]->NotifyDataChange(scada::DataValue{
      scada::Variant{12.5}, {}, start,
      ParseTime("2026-04-20 10:00:01")});
  const auto publish =
      subscription.TryPublish(start + base::TimeDelta::FromMilliseconds(100));
  ASSERT_TRUE(publish.has_value());
  EXPECT_EQ(publish->subscription_id, 17u);
  EXPECT_EQ(publish->available_sequence_numbers,
            (std::vector<scada::UInt32>{1u}));

  EXPECT_EQ(subscription.Acknowledge(std::vector<scada::UInt32>{1u}),
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));
  EXPECT_EQ(subscription.Republish(1u).status.code(),
            scada::StatusCode::Bad_MessageNotAvailable);
}

}  // namespace
}  // namespace opcua

