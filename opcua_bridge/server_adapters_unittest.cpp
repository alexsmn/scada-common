#include "opcua_bridge/server_adapters.h"

#include "opcua/events/event_filter.h"
#include "scada/co_result.h"
#include "scada/history_service.h"
#include "scada/locale_negotiation.h"

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
  event.source_name = u"Pump 42";
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

// Projects `event` through a live subscription adapter created with
// `locale_ids`, subscribed the way the SCADA client subscribes, and decodes the
// one notification on the client side with `packing`.
scada::Event RoundTripLiveEvent(const scada::Event& event,
                                std::vector<std::string> locale_ids,
                                SourceNamePacking packing) {
  auto fake = std::make_unique<FakeMonitoredItemSubscription>();
  fake->next = scada::EventNotification{.item_id = 1,
                                        .client_handle = 55,
                                        .status = scada::StatusCode::Good,
                                        .event = std::any{event}};
  MonitoredItemSubscriptionAdapter adapter{
      std::move(fake),
      opcua::ServiceContext{}.with_locale_ids(std::move(locale_ids)),
      Tracer::None()};

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

  EXPECT_TRUE(read_result.has_value() && read_result->ok() &&
              (*read_result)->size() == 1u);
  if (!read_result.has_value() || !read_result->ok() ||
      (*read_result)->size() != 1u) {
    return {};
  }
  const scada::MonitoredItemNotification decoded =
      ToScada((**read_result)[0], packing);
  const auto* notification = std::get_if<scada::EventNotification>(&decoded);
  const auto* result = notification
                           ? std::any_cast<scada::Event>(&notification->event)
                           : nullptr;
  EXPECT_NE(result, nullptr);
  return result ? *result : scada::Event{};
}

scada::Event EventNamedInTwoLanguages() {
  scada::Event event;
  event.event_type_id = scada::id::SystemEventType;
  event.event_id = 0x42;
  event.time = scada::Now();
  event.severity = 10;
  event.source_node_id = scada::NodeId{5, 2};
  const scada::LocalizedText names[] = {{"ru", u"I ВЛ-110 П"},
                                        {"en", u"I OHL-110 P"}};
  event.source_name = scada::EncodeMultiLanguage(names);
  event.message = scada::LocalizedText{u"Value > 45"};
  return event;
}

// A live event's SourceName crosses a tier hop in every language. It is a
// plain String on the wire (Part 5 §6.4.2), so it stays packed only for a
// subscriber that asked for the private tier tag, and that subscriber's end
// re-attaches the "mul" label. Until this held, the edge flattened every name
// to its first translation for live events — HistoryReadEvents already packed
// — and the proxy served English clients the Russian name. Backlog 819.
TEST(ServerAdapterTest, LiveEventSourceNameStaysPackedAcrossATierHop) {
  const scada::Event received =
      RoundTripLiveEvent(EventNamedInTwoLanguages(),
                         {std::string{scada::kTierMultiLanguageLocale},
                          std::string{scada::kMultiLanguageLocale}},
                         SourceNamePacking::kPacked);

  const std::vector<scada::LocalizedText> names =
      scada::DecodeMultiLanguage(received.source_name);
  ASSERT_EQ(names.size(), 2u);
  EXPECT_EQ(scada::ResolveLocalizedText(received.source_name,
                                        std::vector<std::string>{"en"})
                .text,
            u"I OHL-110 P");
}

// The other half, and the one a packed payload must never cross: any peer that
// did not ask for the tier tag — including a third party legitimately asking
// for "mul" — gets one language, not our JSON.
TEST(ServerAdapterTest, LiveEventSourceNameIsFlattenedForAnyOtherSubscriber) {
  const scada::Event received = RoundTripLiveEvent(
      EventNamedInTwoLanguages(), {std::string{scada::kMultiLanguageLocale}},
      SourceNamePacking::kResolved);

  EXPECT_EQ(received.source_name.text, u"I ВЛ-110 П");
  EXPECT_EQ(scada::DecodeMultiLanguage(received.source_name).size(), 1u);
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

// A HistoryService that returns one event whose message holds every language
// the server could say it in, the way a composed message is stored.
class TwoLanguageHistoryService : public scada::HistoryService {
 public:
  scada::CoStatusOr<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails) override {
    co_return scada::HistoryReadRawResult{};
  }

  scada::CoStatusOr<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId,
      scada::Time,
      scada::Time,
      scada::EventFilter) override {
    const scada::LocalizedText translations[] = {
        {"ru", u"Изменение состояния"},
        {"en", u"State change"},
    };
    const scada::LocalizedText names[] = {
        {"ru", u"Статистика сервера"},
        {"en", u"Server statistics"},
    };
    scada::Event event;
    event.event_id = 1;
    event.time = scada::Now();
    event.severity = scada::kSeverityNormal;
    event.message = scada::EncodeMultiLanguage(translations);
    event.source_name = scada::EncodeMultiLanguage(names);
    co_return scada::HistoryReadEventsResult{.events = {std::move(event)}};
  }
};

// Reads the journal through the adapter under one session's LocaleIds and
// returns the whole event, so both the Message and the SourceName projections
// can be inspected.
// Reads the journal through the adapter under one session's LocaleIds.
opcua::LocalizedText ReadOneEventMessage(std::vector<std::string> locale_ids) {
  TwoLanguageHistoryService fake;
  HistoryServiceAdapter adapter{fake};

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<opcua::HistoryReadEventsResult>> result;
  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        result = co_await adapter.HistoryReadEvents(
            opcua::ServiceContext{}.with_locale_ids(std::move(locale_ids)),
            opcua::NodeId{85u}, opcua::DateTime{}, opcua::DateTime{},
            opcua::EventFilter{});
      },
      boost::asio::detached);
  io.run();

  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->ok());
  EXPECT_EQ((*result)->events.size(), 1u);
  if (!result.has_value() || !result->ok() || (*result)->events.empty())
    return {};
  return (*result)->events.front().message;
}

// The same read, returning the whole event so the SourceName projection can
// be inspected alongside the Message one.
opcua::Event ReadOneEvent(std::vector<std::string> locale_ids) {
  TwoLanguageHistoryService fake;
  HistoryServiceAdapter adapter{fake};

  boost::asio::io_context io;
  std::optional<opcua::StatusOr<opcua::HistoryReadEventsResult>> result;
  boost::asio::co_spawn(
      io,
      [&]() -> opcua::Awaitable<void> {
        result = co_await adapter.HistoryReadEvents(
            opcua::ServiceContext{}.with_locale_ids(std::move(locale_ids)),
            opcua::NodeId{85u}, opcua::DateTime{}, opcua::DateTime{},
            opcua::EventFilter{});
      },
      boost::asio::detached);
  io.run();

  EXPECT_TRUE(result.has_value());
  EXPECT_TRUE(result->ok());
  EXPECT_EQ((*result)->events.size(), 1u);
  if (!result.has_value() || !result->ok() || (*result)->events.empty())
    return {};
  return (*result)->events.front();
}

// The session's LocaleIds choose which stored translation crosses the wire.
// OPC UA Part 4 §5.4 Locale Negotiation,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/5.4 — Part 11
// states no exception for historical access, so reading the journal is
// localized exactly like receiving the event live.
TEST(ServerAdapterTest, HistoryEventMessageIsResolvedToTheSessionLanguage) {
  const auto english = ReadOneEventMessage({"en"});
  EXPECT_EQ(english.locale, "en");
  EXPECT_EQ(english.text, u"State change");

  const auto russian = ReadOneEventMessage({"ru"});
  EXPECT_EQ(russian.locale, "ru");
  EXPECT_NE(russian.text, u"State change");
  EXPECT_FALSE(russian.text.empty());
}

// A session that named no locale must not receive the packed payload: it
// would render as JSON in the event journal.
TEST(ServerAdapterTest, HistoryEventMessageIsNeverThePackedFormOnTheWire) {
  const auto received = ReadOneEventMessage({});
  EXPECT_NE(received.locale, std::string{scada::kMultiLanguageLocale});
  EXPECT_EQ(received.text.find(u"{\"t\":"), std::u16string::npos);
}

// SourceName follows the session as well as Message does. It is the source
// node's DisplayName as it stood when the event was produced, carried with
// every language that name had, and Part 5 §6.4.2 makes the wire field a
// plain `String` — so the difference between two sessions is the string.
//
// Regression test for backlog 802.
TEST(ServerAdapterTest, HistoryEventSourceNameIsResolvedToTheSessionLanguage) {
  const auto english = ReadOneEvent({"en"});
  EXPECT_EQ(english.source_name, "Server statistics");

  const auto russian = ReadOneEvent({"ru"});
  EXPECT_NE(russian.source_name, "Server statistics");
  EXPECT_FALSE(russian.source_name.empty());
}

// A packed payload reaching the wire would render as raw JSON in a journal's
// object column, which is worse than the wrong language.
TEST(ServerAdapterTest, HistoryEventSourceNameIsNeverThePackedForm) {
  const auto received = ReadOneEvent({});
  EXPECT_FALSE(received.source_name.empty());
  EXPECT_EQ(received.source_name.find("{\"t\":"), std::string::npos);
}

// --- SourceName across a tier hop (backlog 819) ------------------------

// Our own tiers ask with the private tag, and only they may have the packed
// payload: they know how to put the "mul" marker back on a field that could not
// carry it. The hop is what this buys — without it the downstream tier holds
// one language and every client it serves gets that one, whatever it asked for.
TEST(ServerAdapterTest, HistoryEventSourceNameStaysPackedForATierSession) {
  const auto received =
      ReadOneEvent({std::string{scada::kTierMultiLanguageLocale},
                    std::string{scada::kMultiLanguageLocale}});

  ASSERT_NE(received.source_name.find("{\"t\":"), std::string::npos)
      << "a tier session must receive the packed payload: "
      << received.source_name;
  // Both languages, so the downstream tier can resolve per client session.
  EXPECT_NE(received.source_name.find("Server statistics"), std::string::npos);
  EXPECT_NE(received.source_name.find("\"ru\""), std::string::npos);
}

// And the test that matters more, because it is the one that was shipped wrong.
// `mul` is a locale any third-party client may legally ask for — it is asking
// for multi-language LocalizedText values, which it is entitled to — and it
// says nothing about being able to unpack a `String`. 50cd12402 packed for
// everyone on the reasoning that asking for "mul" was licence enough, and the
// demo served raw JSON in the journal's object column to every session;
// 96f99d7ab reverted it. Only the private tag licenses packing.
TEST(ServerAdapterTest, HistoryEventSourceNameIsNotPackedForAPlainMulSession) {
  const auto received =
      ReadOneEvent({std::string{scada::kMultiLanguageLocale}});

  EXPECT_FALSE(received.source_name.empty());
  EXPECT_EQ(received.source_name.find("{\"t\":"), std::string::npos)
      << "a plain \"mul\" session must not receive packed JSON in SourceName: "
      << received.source_name;
}

// The receiving half, which is where the marker goes back on. A packed
// SourceName crossing as a bare `String` is only recoverable because this end
// knows its own session asked for the tag; with `kResolved` the same bytes are
// one opaque language, which is the pre-819 behaviour and still the default.
TEST(ServerAdapterTest, APackedSourceNameSurvivesTheRoundTripOnlyWhenLabelled) {
  const scada::LocalizedText names[] = {
      {"ru", u"Статистика сервера"},
      {"en", u"Server statistics"},
  };
  scada::Event event;
  event.source_name = scada::EncodeMultiLanguage(names);

  const opcua::Event on_the_wire = ToOpcua(event, SourceNamePacking::kPacked);
  const scada::Event recovered =
      ToScada(on_the_wire, SourceNamePacking::kPacked);
  EXPECT_EQ(scada::ResolveLocalizedText(recovered.source_name,
                                        std::vector<scada::String>{"en"})
                .text,
            u"Server statistics");
  EXPECT_EQ(scada::ResolveLocalizedText(recovered.source_name,
                                        std::vector<scada::String>{"ru"})
                .text,
            u"Статистика сервера");

  // Unlabelled, the same wire bytes stay a single opaque value: resolution
  // cannot tell there was ever more than one language in there.
  const scada::Event unlabelled =
      ToScada(on_the_wire, SourceNamePacking::kResolved);
  EXPECT_TRUE(unlabelled.source_name.locale.empty());
  EXPECT_EQ(scada::DecodeMultiLanguage(unlabelled.source_name).size(), 1u);
}

// Labelling a plain name as packed is the mislabelling this design tolerates on
// purpose: an upstream that ignores the private tag answers with one language,
// and the receiver has already committed to `kPacked`. The name must survive.
TEST(ServerAdapterTest, LabellingAnUnpackedSourceNameKeepsTheNameIntact) {
  scada::Event event;
  event.source_name = scada::LocalizedText{"en", u"Server statistics"};

  const scada::Event recovered = ToScada(
      ToOpcua(event, SourceNamePacking::kResolved), SourceNamePacking::kPacked);

  EXPECT_EQ(scada::ResolveLocalizedText(recovered.source_name,
                                        std::vector<scada::String>{"en"})
                .text,
            u"Server statistics");
}

}  // namespace
}  // namespace scada::opcua_bridge
