#include "opcua_bridge/server_adapters.h"

#include "opcua/events/event_filter.h"

#include <gtest/gtest.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>

#include <any>
#include <optional>
#include <variant>

namespace opcua_bridge {
namespace {

// A core ViewService that records the (converted) request it receives and
// returns a fixed result, so the adapter's both-directions conversion and
// delegation can be observed.
class FakeViewService : public scada::ViewService {
 public:
  Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>> Browse(
      scada::ServiceContext,
      std::vector<scada::BrowseDescription> inputs) override {
    received_inputs = std::move(inputs);
    scada::BrowseResult result;
    result.status_code = scada::StatusCode::Good;
    result.references.push_back(
        scada::ReferenceDescription{.node_id = scada::NodeId{2253u}});
    co_return std::vector<scada::BrowseResult>{std::move(result)};
  }
  Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath>) override {
    co_return std::vector<scada::BrowsePathResult>{};
  }

  std::vector<scada::BrowseDescription> received_inputs;
};

TEST(ServerAdapterTest, ViewServiceBrowseDelegatesAndConverts) {
  FakeViewService fake;
  ViewServiceAdapter adapter{fake};

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<std::vector<opcua::BrowseResult>>> result;

  // Call the adapter through its opcua interface with opcua-typed input.
  std::vector<opcua::BrowseDescription> inputs;
  inputs.push_back(
      opcua::BrowseDescription{.node_id = opcua::NodeId{84u},
                               .direction = opcua::BrowseDirection::Forward});

  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        result = co_await adapter.Browse(opcua::ServiceContext{}, inputs);
      },
      boost::asio::detached);
  io.run();

  // The adapter converted opcua input -> scada and delegated to the fake.
  ASSERT_EQ(fake.received_inputs.size(), 1u);
  EXPECT_EQ(fake.received_inputs[0].node_id, scada::NodeId{84u});
  EXPECT_EQ(fake.received_inputs[0].direction, scada::BrowseDirection::Forward);

  // The adapter converted the scada result back to opcua.
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->ok());
  ASSERT_EQ((*result)->size(), 1u);
  const auto& browse_result = (**result)[0];
  EXPECT_EQ(browse_result.status_code, opcua::StatusCode::Good);
  ASSERT_EQ(browse_result.references.size(), 1u);
  EXPECT_EQ(browse_result.references[0].node_id, opcua::NodeId{2253u});
}

// A core MonitoredItemSubscription whose ReadNext returns a single, fixed
// notification supplied by the test, so the adapter's core->wire conversion can
// be observed.
class FakeMonitoredItemSubscription : public scada::MonitoredItemSubscription {
 public:
  Awaitable<std::vector<scada::MonitoredItemCreateResult>> AddItems(
      std::vector<scada::MonitoredItemCreateRequest> requests) override {
    std::vector<scada::MonitoredItemCreateResult> results;
    for (const auto& request : requests)
      results.push_back({.item_id = 1,
                         .client_handle = request.client_handle,
                         .status = scada::StatusCode::Good});
    co_return results;
  }
  Awaitable<std::vector<scada::Status>> RemoveItems(
      std::span<const scada::MonitoredItemId>) override {
    co_return std::vector<scada::Status>{};
  }
  Awaitable<scada::StatusOr<std::vector<scada::MonitoredItemNotification>>>
  ReadNext(std::size_t) override {
    std::vector<scada::MonitoredItemNotification> out;
    out.push_back(next);
    co_return out;
  }
  void Close(scada::Status) override {}

  scada::MonitoredItemNotification next;
};

// A populated core EventNotification routed through the bridge as a wire
// EventFieldList must carry the real field values projected onto the
// EventFilter select clauses ({Message},{Severity},{EventId}).
TEST(ServerAdapterTest, EventNotificationProjectsRealFieldValuesToOpcua) {
  auto fake = std::make_unique<FakeMonitoredItemSubscription>();
  auto* fake_ptr = fake.get();

  scada::Event event;
  event.event_id = 77;
  event.event_type_id = scada::id::SystemEventType;
  event.node_id = scada::NodeId{3001u};
  event.message = scada::LocalizedText{u"custom alarm"};
  event.severity = 600;
  fake_ptr->next = scada::EventNotification{.item_id = 1,
                                            .client_handle = 55,
                                            .status = scada::StatusCode::Good,
                                            .event = std::any{event}};

  MonitoredItemSubscriptionAdapter adapter{std::move(fake)};

  // Add the item with an EventFilter selecting three fields so the adapter
  // stores the field paths keyed by client_handle.
  opcua::MonitoredItemCreateRequest request;
  request.item_to_monitor = {.node_id = opcua::NodeId{2253u},
                             .attribute_id = opcua::AttributeId::EventNotifier};
  request.requested_parameters.client_handle = 55;
  request.requested_parameters.filter = opcua::MonitoringFilter{
      opcua::BuildEventFilter(std::vector<std::vector<std::string>>{
          {"Message"}, {"Severity"}, {"EventId"}})};

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<std::vector<opcua::ItemNotification>>>
      read_result;
  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        std::vector<opcua::MonitoredItemCreateRequest> requests;
        requests.push_back(request);
        co_await adapter.AddItems(std::move(requests));
        read_result = co_await adapter.ReadNext(10);
      },
      boost::asio::detached);
  io.run();

  ASSERT_TRUE(read_result.has_value());
  ASSERT_TRUE(read_result->ok());
  ASSERT_EQ((*read_result)->size(), 1u);
  const auto* event_fields =
      std::get_if<opcua::EventFieldList>(&(**read_result)[0]);
  ASSERT_NE(event_fields, nullptr);
  EXPECT_EQ(event_fields->client_handle, 55u);
  ASSERT_EQ(event_fields->event_fields.size(), 3u);
  EXPECT_EQ(event_fields->event_fields[0].get<opcua::LocalizedText>(),
            opcua::LocalizedText{u"custom alarm"});
  EXPECT_EQ(event_fields->event_fields[1].get<opcua::UInt32>(), 600u);
  EXPECT_EQ(event_fields->event_fields[2].get<opcua::UInt64>(), 77u);
}

}  // namespace
}  // namespace opcua_bridge
