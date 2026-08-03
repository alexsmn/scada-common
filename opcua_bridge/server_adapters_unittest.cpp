#include "opcua_bridge/server_adapters.h"

#include "opcua/events/event_filter.h"
#include "scada/co_result.h"

#include <gtest/gtest.h>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>

#include <any>
#include <optional>
#include <variant>

namespace scada::opcua_bridge {
namespace {

// A core ViewService that records the (converted) request it receives and
// returns a fixed result, so the adapter's both-directions conversion and
// delegation can be observed.
class FakeViewService : public scada::ViewService {
 public:
  scada::CoStatusOr<std::vector<scada::BrowseResult>> Browse(
      scada::ServiceContext,
      std::vector<scada::BrowseDescription> inputs) override {
    received_inputs = std::move(inputs);
    scada::BrowseResult result;
    result.status_code = scada::StatusCode::Good;
    result.references.push_back(
        scada::ReferenceDescription{.node_id = scada::NodeId{2253u}});
    co_return std::vector<scada::BrowseResult>{std::move(result)};
  }
  scada::CoStatusOr<std::vector<scada::BrowsePathResult>> TranslateBrowsePaths(
      std::vector<scada::BrowsePath>) override {
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
  scada::CoStatusOr<std::vector<scada::MonitoredItemNotification>> ReadNext(
      std::size_t) override {
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
  event.source_node_id = scada::NodeId{3001u};
  event.message = scada::LocalizedText{u"custom alarm"};
  event.severity = 600;
  fake_ptr->next = scada::EventNotification{.item_id = 1,
                                            .client_handle = 55,
                                            .status = scada::StatusCode::Good,
                                            .event = std::any{event}};

  MonitoredItemSubscriptionAdapter adapter{
      std::move(fake), opcua::ServiceContext{}, Tracer::None()};

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
  EXPECT_EQ(event_fields->event_fields[1].get<opcua::UInt16>(), 600u);
  // EventId is projected as ByteString per OPC UA Part 5 §6.4.2 BaseEventType,
  // https://reference.opcfoundation.org/Core/Part5/v105/docs/6.4.2
  EXPECT_EQ(event_fields->event_fields[2].get<opcua::ByteString>(),
            opcua::EncodeEventIdByteString(77));
}

// An event notification with NO payload is the monitored item's initial status
// report, not an event, and has no wire representation. Publishing it would put
// one null Variant per select clause on the wire, which a peer cannot tell
// apart from a real event whose fields are all null — that is how the
// aggregating proxy came to relay a phantom event with a zero EventId and
// panic. It must be dropped, not projected.
TEST(ServerAdapterTest, PayloadlessEventNotificationIsNotPublished) {
  auto fake = std::make_unique<FakeMonitoredItemSubscription>();
  auto* fake_ptr = fake.get();
  fake_ptr->next = scada::EventNotification{.item_id = 1,
                                            .client_handle = 55,
                                            .status = scada::StatusCode::Good,
                                            .event = std::any{}};

  MonitoredItemSubscriptionAdapter adapter{
      std::move(fake), opcua::ServiceContext{}, Tracer::None()};

  opcua::MonitoredItemCreateRequest request;
  request.item_to_monitor = {.node_id = opcua::NodeId{2253u},
                             .attribute_id = opcua::AttributeId::EventNotifier};
  request.requested_parameters.client_handle = 55;

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
  EXPECT_TRUE((*read_result)->empty());
}

// A scada::Event crossing the SCADA-to-SCADA path — an event filter WITHOUT
// select clauses, so the serving side projects the default full-fidelity
// field paths — must reconstruct on the client side with identity (event id,
// receive time) and payload intact. The aggregation event tap forwards
// downstream process/alarm events through exactly this path (ADR 0004).
TEST(ServerAdapterTest, ScadaEventRoundTripsThroughDefaultProjection) {
  auto fake = std::make_unique<FakeMonitoredItemSubscription>();
  auto* fake_ptr = fake.get();

  scada::Event event;
  event.event_type_id = scada::id::SystemEventType;
  event.event_id = 0x123456789;
  event.time = scada::Now();
  event.receive_time = scada::Now();
  event.change_mask = scada::Event::EVT_VAL;
  event.severity = 600;
  event.source_node_id = scada::NodeId{42, 2};
  // Post-producer events carry a resolved SourceName; an empty one would
  // round-trip as the projection's NodeId-string fallback, not as empty.
  event.source_name = "Pump 42";
  event.user_id = scada::NodeId{7, 3};
  event.value = scada::Variant{123};
  event.message = scada::LocalizedText{u"forwarded alarm"};
  fake_ptr->next = scada::EventNotification{.item_id = 1,
                                            .client_handle = 55,
                                            .status = scada::StatusCode::Good,
                                            .event = std::any{event}};

  MonitoredItemSubscriptionAdapter adapter{
      std::move(fake), opcua::ServiceContext{}, Tracer::None()};

  // Subscribe the way the SCADA client does: a scada::EventFilter converts to
  // the `_scada` json wire filter, which carries no SelectClauses.
  scada::MonitoringParameters scada_params;
  scada_params.filter = scada::EventFilter{
      .of_type = {scada::NodeId{scada::id::SystemEventType}}};
  opcua::MonitoredItemCreateRequest request;
  request.item_to_monitor = {.node_id = opcua::NodeId{2253u},
                             .attribute_id = opcua::AttributeId::EventNotifier};
  request.requested_parameters = ToOpcua(scada_params);
  request.requested_parameters.client_handle = 55;

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

  // Client-side decode: the default projection reconstructs the full event.
  const scada::MonitoredItemNotification decoded = ToScada((**read_result)[0]);
  const auto* decoded_event = std::get_if<scada::EventNotification>(&decoded);
  ASSERT_NE(decoded_event, nullptr);
  EXPECT_EQ(decoded_event->client_handle, 55u);
  const auto* reconstructed =
      std::any_cast<scada::Event>(&decoded_event->event);
  ASSERT_NE(reconstructed, nullptr);
  EXPECT_EQ(*reconstructed, event);
}

// A core AttributeService returning caller-supplied Read results, so the
// adapter's outbound status projection can be observed.
class FakeAttributeService : public scada::AttributeService {
 public:
  scada::CoStatusOr<std::vector<scada::DataValue>> Read(
      scada::ServiceContext,
      std::vector<scada::ReadValueId> inputs) override {
    received_inputs = std::move(inputs);
    co_return results;
  }
  scada::CoStatusOr<std::vector<scada::StatusCode>> Write(
      scada::ServiceContext,
      std::vector<scada::WriteValue>) override {
    co_return std::vector<scada::StatusCode>{};
  }

  std::vector<scada::ReadValueId> received_inputs;
  std::vector<scada::DataValue> results;
};

// The wire-side bug this guards: an item whose data source has never delivered
// anything read back as `Value: (empty) Type: (empty) Status: Good`. An empty
// Variant must go out at Bad_WaitingForInitialData instead — OPC UA Part 4
// §7.38.2, https://reference.opcfoundation.org/Core/Part4/v105/docs/7.38.2
TEST(ServerAdapterTest, ReadOfValuelessItemIsNotReportedGood) {
  FakeAttributeService fake;
  // The demo shape: timestamps stamped, quality bits already offline, no value.
  scada::DataValue no_value;
  no_value.qualifier.set_online(false);
  no_value.source_timestamp = scada::Now();
  no_value.server_timestamp = no_value.source_timestamp;
  fake.results.push_back(no_value);

  AttributeServiceAdapter adapter{fake};

  auto inputs = std::make_shared<std::vector<opcua::ReadValueId>>();
  inputs->push_back({.node_id = opcua::NodeId{std::string{"1"}, 2},
                     .attribute_id = opcua::AttributeId::Value});

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<std::vector<opcua::DataValue>>> result;
  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        result = co_await adapter.Read(opcua::ServiceContext{}, inputs);
      },
      boost::asio::detached);
  io.run();

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->ok());
  ASSERT_EQ((*result)->size(), 1u);
  const auto& value = (**result)[0];
  EXPECT_TRUE(value.value.is_null());
  EXPECT_EQ(value.status_code, opcua::StatusCode::Bad_WaitingForInitialData);
  // Bad severity is what a spec-conforming client keys off.
  EXPECT_TRUE(opcua::IsBad(value.status_code));
  // The wire value is the standard BadWaitingForInitialData.
  EXPECT_EQ(opcua::Status{value.status_code}.full_code(), 0x80320000u);
}

// A real value keeps its status; the projection only fills the value-less gap.
TEST(ServerAdapterTest, ReadOfDeliveredValueStaysGood) {
  FakeAttributeService fake;
  const auto now = scada::Now();
  fake.results.push_back(scada::DataValue{scada::Variant{42}, {}, now, now});

  AttributeServiceAdapter adapter{fake};

  auto inputs = std::make_shared<std::vector<opcua::ReadValueId>>();
  inputs->push_back({.node_id = opcua::NodeId{std::string{"1"}, 2},
                     .attribute_id = opcua::AttributeId::Value});

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<std::vector<opcua::DataValue>>> result;
  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        result = co_await adapter.Read(opcua::ServiceContext{}, inputs);
      },
      boost::asio::detached);
  io.run();

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->ok());
  ASSERT_EQ((*result)->size(), 1u);
  EXPECT_EQ((**result)[0].status_code, opcua::StatusCode::Good);
  EXPECT_EQ((**result)[0].value.get<opcua::Int32>(), 42);
}

// A delivered value from an offline device: the SCADA Qualifier rides along as
// an opcuapp extension field that only a SCADA peer decodes, so the quality has
// to reach a standard client as the StatusCode. Uncertain severity, wire value
// per the bridge's status mapping.
TEST(ServerAdapterTest, ReadOfStaleValueCarriesQualityAsStatus) {
  FakeAttributeService fake;
  const auto now = scada::Now();
  fake.results.push_back(
      scada::DataValue{scada::Variant{42},
                       scada::Qualifier{scada::Qualifier::OFFLINE}, now, now});

  AttributeServiceAdapter adapter{fake};

  auto inputs = std::make_shared<std::vector<opcua::ReadValueId>>();
  inputs->push_back({.node_id = opcua::NodeId{std::string{"1"}, 2},
                     .attribute_id = opcua::AttributeId::Value});

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<std::vector<opcua::DataValue>>> result;
  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        result = co_await adapter.Read(opcua::ServiceContext{}, inputs);
      },
      boost::asio::detached);
  io.run();

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->ok());
  ASSERT_EQ((*result)->size(), 1u);
  const auto& value = (**result)[0];
  // The value itself still travels — Uncertain means "might not be suitable for
  // some purposes", not "absent".
  EXPECT_EQ(value.value.get<opcua::Int32>(), 42);
  EXPECT_EQ(value.status_code, opcua::StatusCode::Uncertain_Disconnected);
  EXPECT_EQ(static_cast<int>(opcua::GetSeverity(value.status_code)),
            static_cast<int>(opcua::StatusSeverity::Uncertain));
}

// Non-Value attributes are untouched: their absence is the service's own
// business (Bad_WrongAttributeId and friends), not a missing data source.
TEST(ServerAdapterTest, ReadOfNonValueAttributeIsNotRewritten) {
  FakeAttributeService fake;
  fake.results.push_back(scada::DataValue{});

  AttributeServiceAdapter adapter{fake};

  auto inputs = std::make_shared<std::vector<opcua::ReadValueId>>();
  inputs->push_back({.node_id = opcua::NodeId{std::string{"1"}, 2},
                     .attribute_id = opcua::AttributeId::Description});

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<std::vector<opcua::DataValue>>> result;
  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        result = co_await adapter.Read(opcua::ServiceContext{}, inputs);
      },
      boost::asio::detached);
  io.run();

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->ok());
  ASSERT_EQ((*result)->size(), 1u);
  EXPECT_EQ((**result)[0].status_code, opcua::StatusCode::Good);
}

// The subscription path is the one the 25 s watch exercised: 44 notifications,
// every one Good over nothing. Each published sample gets the same projection.
TEST(ServerAdapterTest, DataChangeNotificationOfValuelessItemIsNotGood) {
  auto fake = std::make_unique<FakeMonitoredItemSubscription>();
  auto* fake_ptr = fake.get();

  scada::DataValue no_value;
  no_value.qualifier.set_online(false);
  no_value.source_timestamp = scada::Now();
  no_value.server_timestamp = no_value.source_timestamp;
  fake_ptr->next = scada::DataChangeNotification{
      .item_id = 1, .client_handle = 55, .value = no_value};

  MonitoredItemSubscriptionAdapter adapter{
      std::move(fake), opcua::ServiceContext{}, Tracer::None()};

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<std::vector<opcua::ItemNotification>>>
      read_result;
  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        read_result = co_await adapter.ReadNext(10);
      },
      boost::asio::detached);
  io.run();

  ASSERT_TRUE(read_result.has_value());
  ASSERT_TRUE(read_result->ok());
  ASSERT_EQ((*read_result)->size(), 1u);
  const auto* notification =
      std::get_if<opcua::MonitoredItemNotification>(&(**read_result)[0]);
  ASSERT_NE(notification, nullptr);
  EXPECT_EQ(notification->client_handle, 55u);
  EXPECT_TRUE(notification->value.value.is_null());
  EXPECT_EQ(notification->value.status_code,
            opcua::StatusCode::Bad_WaitingForInitialData);
}

// A delivered value publishes unchanged.
TEST(ServerAdapterTest, DataChangeNotificationOfDeliveredValueStaysGood) {
  auto fake = std::make_unique<FakeMonitoredItemSubscription>();
  auto* fake_ptr = fake.get();

  const auto now = scada::Now();
  fake_ptr->next = scada::DataChangeNotification{
      .item_id = 1,
      .client_handle = 55,
      .value = scada::DataValue{scada::Variant{1.5}, {}, now, now}};

  MonitoredItemSubscriptionAdapter adapter{
      std::move(fake), opcua::ServiceContext{}, Tracer::None()};

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<std::vector<opcua::ItemNotification>>>
      read_result;
  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        read_result = co_await adapter.ReadNext(10);
      },
      boost::asio::detached);
  io.run();

  ASSERT_TRUE(read_result.has_value());
  ASSERT_TRUE(read_result->ok());
  ASSERT_EQ((*read_result)->size(), 1u);
  const auto* notification =
      std::get_if<opcua::MonitoredItemNotification>(&(**read_result)[0]);
  ASSERT_NE(notification, nullptr);
  EXPECT_EQ(notification->value.status_code, opcua::StatusCode::Good);
  EXPECT_DOUBLE_EQ(notification->value.value.get<opcua::Double>(), 1.5);
}

}  // namespace
}  // namespace scada::opcua_bridge
