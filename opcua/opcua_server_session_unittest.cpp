#include "opcua/opcua_server_session.h"

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
    (void)value_id;
    (void)params;
    auto item = std::make_shared<scada::TestMonitoredItem>();
    items.push_back(item);
    return item;
  }

  std::vector<std::shared_ptr<scada::TestMonitoredItem>> items;
};

TEST(OpcUaServerSessionTest, StoresContinuationPointsAndTransfersSubscriptions) {
  TestMonitoredItemService monitored_item_service;
  auto now = ParseTime("2026-04-20 17:00:00");
  OpcUaSession source{{
      .session_id = NumericNode(1001),
      .authentication_token = NumericNode(2001, 3),
      .service_context = scada::ServiceContext{}.with_user_id(NumericNode(77, 4)),
      .monitored_item_service = monitored_item_service,
      .now = [&] { return now; },
  }};
  OpcUaSession target{{
      .session_id = NumericNode(1002),
      .authentication_token = NumericNode(2002, 3),
      .service_context = scada::ServiceContext{}.with_user_id(NumericNode(78, 4)),
      .monitored_item_service = monitored_item_service,
      .now = [&] { return now; },
  }};

  const auto created = source.CreateSubscription(
      {.parameters = {.publishing_interval_ms = 100,
                      .lifetime_count = 60,
                      .max_keep_alive_count = 3,
                      .publishing_enabled = true}});
  EXPECT_TRUE(source.HasSubscription(created.subscription_id));

  auto paged = source.StoreBrowseResults(
      {.status = scada::StatusCode::Good,
       .results = {{.status_code = scada::StatusCode::Good,
                    .references = {{.reference_type_id = NumericNode(501),
                                    .forward = true,
                                    .node_id = NumericNode(601)},
                                   {.reference_type_id = NumericNode(502),
                                    .forward = true,
                                    .node_id = NumericNode(602)}}}}},
      1);
  ASSERT_EQ(paged.results.size(), 1u);
  ASSERT_FALSE(paged.results[0].continuation_point.empty());

  const auto next = source.BrowseNext(
      {.continuation_points = {paged.results[0].continuation_point}});
  ASSERT_EQ(next.results.size(), 1u);
  EXPECT_EQ(next.results[0].status_code, scada::StatusCode::Good);
  ASSERT_EQ(next.results[0].references.size(), 1u);
  EXPECT_EQ(next.results[0].references[0].node_id, NumericNode(602));

  const auto transferred = target.TransferSubscriptionsFrom(
      source,
      {.subscription_ids = {created.subscription_id}, .send_initial_values = true});
  EXPECT_EQ(transferred.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));
  EXPECT_FALSE(source.HasSubscription(created.subscription_id));
  EXPECT_TRUE(target.HasSubscription(created.subscription_id));
}

}  // namespace
}  // namespace opcua
