#include "events/event_fetcher.h"

#include "base/boost_log.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "events/event_ack_queue.h"
#include "events/event_observer.h"
#include "events/event_storage.h"
#include "model/devices_node_ids.h"
#include "scada/co_result.h"
#include "scada/history_service.h"
#include "scada/item_factory_subscription.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "scada/standard_node_ids.h"
#include "scada/status.h"
#include "scada/test/test_monitored_item.h"

#include <gtest/gtest.h>

#include <any>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {

class TestEventObserver final : public EventObserver {
 public:
  void OnEvents(std::span<const scada::Event* const> events) override {
    events_called = true;
    event_count += events.size();
  }

  bool events_called = false;
  std::size_t event_count = 0;
};

// MonitoredItemService backed by the production subscription adapter
// (`MakeItemFactorySubscription`). Each subscription creates its items through
// a shared `TestMonitoredItem`, so a test can push a system event down the real
// delivery path: TestMonitoredItem -> ItemFactorySubscription -> subscription
// pump -> LegacyMonitoredItemAdapter -> EventFetcher.
class FakeMonitoredItemService final : public scada::MonitoredItemService {
 public:
  scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
  CreateSubscription(scada::ServiceContext /*context*/,
                     scada::MonitoredItemSubscriptionOptions options) override {
    return scada::MakeItemFactorySubscription(
        [this](const scada::ReadValueId& /*value_id*/,
               const scada::MonitoringParameters& params) {
          auto item = std::make_shared<scada::TestMonitoredItem>();
          items.push_back(item);
          this->params.push_back(params);
          return item;
        },
        options);
  }

  std::vector<std::shared_ptr<scada::TestMonitoredItem>> items;
  std::vector<scada::MonitoringParameters> params;
};

// HistoryService that reports no unacked history. `EventFetcher` only reads
// history from `OnChannelOpened`, which these tests do not exercise.
class FakeHistoryService final : public scada::HistoryService {
 public:
  scada::CoStatusOr<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails /*details*/) override {
    co_return scada::HistoryReadRawResult{};
  }

  scada::CoStatusOr<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::ServiceContext,
      scada::NodeId /*node_id*/,
      scada::Time /*from*/,
      scada::Time /*to*/,
      scada::EventFilter /*filter*/) override {
    co_return scada::HistoryReadEventsResult{};
  }
};

// MethodService that accepts any call and records it.
class FakeMethodService final : public scada::MethodService {
 public:
  struct Call_ {
    scada::NodeId node_id;
    scada::NodeId method_id;
    std::vector<scada::Variant> arguments;
  };

  scada::CoStatusOr<scada::CallResult> Call(
      scada::NodeId node_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments,
      scada::ServiceContext /*context*/) override {
    calls.push_back({node_id, method_id, arguments});
    co_return scada::CallResult{};
  }

  std::vector<Call_> calls;
};

scada::Event MakeEvent(scada::EventId event_id,
                       const scada::NodeId& node_id,
                       scada::EventSeverity severity = scada::kSeverityNormal) {
  scada::Event event;
  event.event_id = event_id;
  event.source_node_id = node_id;
  event.severity = severity;
  return event;
}

struct TestContext {
  TestContext()
      : ack_queue{EventAckQueueContext{
            .logger_ = std::make_shared<BoostLogger>(LOG_NAME("Test")),
            .executor_ = executor,
            .method_service_ = method_service}} {}

  // Constructs the fetcher and drives it until its system-events monitored item
  // is established. `EventFetcher` subscribes lazily through the real
  // `CreateSubscription` path on the executor, so a drain is needed before the
  // fake item exists.
  void StartFetcher(EventObserver& observer, bool foreign_server = false) {
    fetcher.emplace(EventFetcherContext{
        .executor_ = executor,
        .monitored_item_service_ = monitored_item_service,
        .history_service_ = history_service,
        .logger_ = std::make_shared<BoostLogger>(LOG_NAME("Test")),
        .event_storage_ = event_storage,
        .event_ack_queue_ = ack_queue,
        .foreign_server_ = foreign_server});
    fetcher->AddObserver(observer);
    Drain(executor);
  }

  // The single system-events monitored item created by the fetcher. Valid only
  // after `StartFetcher`.
  scada::TestMonitoredItem& system_events_item() {
    return *monitored_item_service.items.at(0);
  }

  TestExecutor executor;
  FakeMonitoredItemService monitored_item_service;
  FakeHistoryService history_service;
  FakeMethodService method_service;
  EventStorage event_storage;
  EventAckQueue ack_queue;
  std::optional<EventFetcher> fetcher;
};

}  // namespace

TEST(EventFetcherTest, MonitoredItemEventsAreDeliveredThroughCoroutineTask) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  TestEventObserver observer;

  context.StartFetcher(observer);
  ASSERT_EQ(context.monitored_item_service.items.size(), 1u);

  context.system_events_item().NotifyEvent(std::any{MakeEvent(42, node_id)});

  // Delivery is deferred to a coroutine task posted on the executor.
  EXPECT_FALSE(observer.events_called);
  Drain(context.executor);

  EXPECT_TRUE(observer.events_called);
  EXPECT_EQ(observer.event_count, 1u);
  EXPECT_TRUE(context.fetcher->IsAlerting(node_id));
}

TEST(EventFetcherTest, MonitoredItemEventsAreCanceledAfterDestroy) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  TestEventObserver observer;

  context.StartFetcher(observer);
  ASSERT_EQ(context.monitored_item_service.items.size(), 1u);

  scada::TestMonitoredItem& item = context.system_events_item();
  context.fetcher.reset();

  item.NotifyEvent(std::any{MakeEvent(42, node_id)});
  Drain(context.executor);

  EXPECT_FALSE(observer.events_called);
  EXPECT_TRUE(context.event_storage.events().empty());
}

// The device protocol trace and the operator's event journal are separate
// surfaces (the `deviceWatch` row in docs/parity/qt-web-parity.json versus
// UC-5 process events), and nothing on the wire keeps them apart:
// DeviceWatchEventType subtypes SystemEventType, which is exactly what this
// fetcher's EventFilter asks for, and OPC UA event filters match subtypes. So
// the journal filled with raw IEC-104 APDUs from any link that was merely
// alive — observed 2026-08-22 against a standalone scada-iec104.
TEST(EventFetcherTest, DeviceWatchEventsAreNotDeliveredToTheJournal) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  TestEventObserver observer;

  context.StartFetcher(observer);
  ASSERT_EQ(context.monitored_item_service.items.size(), 1u);

  scada::Event event = MakeEvent(42, node_id);
  event.event_type_id = scada::devices::id::DeviceWatchEventType;
  event.message = u"[TCP Accepted #3]: [dev:0] #RX: IEC-104: U_TESTFR_CON";
  context.system_events_item().NotifyEvent(std::any{event});
  Drain(context.executor);

  EXPECT_FALSE(observer.events_called);
  EXPECT_TRUE(context.event_storage.events().empty());
  EXPECT_FALSE(context.fetcher->IsAlerting(node_id));
}

// DeviceFrameEventType subtypes DeviceWatchEventType, so a check that knew
// only its supertype would still let a decoded frame through the history path,
// which assembles plain scada::Events rather than scada::DeviceFrameEvents.
TEST(EventFetcherTest, DeviceFrameEventsAreNotDeliveredToTheJournal) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  TestEventObserver observer;

  context.StartFetcher(observer);
  ASSERT_EQ(context.monitored_item_service.items.size(), 1u);

  scada::Event event = MakeEvent(43, node_id);
  event.event_type_id = scada::devices::id::DeviceFrameEventType;
  event.message = u"Accepted: #RX: 68048300000064010600010000000014000000";
  context.system_events_item().NotifyEvent(std::any{event});
  Drain(context.executor);

  EXPECT_FALSE(observer.events_called);
  EXPECT_TRUE(context.event_storage.events().empty());
}

// The exclusion is stated by type, so it must not narrow the journal to a
// whitelist: an event of any other type still reaches it.
TEST(EventFetcherTest, ProcessEventsStillReachTheJournalAlongsideDeviceEvents) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  TestEventObserver observer;

  context.StartFetcher(observer);
  ASSERT_EQ(context.monitored_item_service.items.size(), 1u);

  scada::Event trace = MakeEvent(44, node_id);
  trace.event_type_id = scada::devices::id::DeviceWatchEventType;
  context.system_events_item().NotifyEvent(std::any{trace});

  scada::Event process = MakeEvent(45, node_id);
  process.event_type_id = scada::NodeId{scada::id::SystemEventType};
  process.message = u"Pressure high";
  context.system_events_item().NotifyEvent(std::any{process});

  Drain(context.executor);

  EXPECT_TRUE(observer.events_called);
  EXPECT_EQ(observer.event_count, 1u);
  EXPECT_EQ(context.event_storage.events().size(), 1u);
}

TEST(EventFetcherTest, MonitoredItemEventWithBadStatusIsIgnored) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  TestEventObserver observer;

  context.StartFetcher(observer);
  ASSERT_EQ(context.monitored_item_service.items.size(), 1u);

  context.system_events_item().NotifyEvent(
      std::any{MakeEvent(42, node_id)}, scada::Status{scada::StatusCode::Bad});
  Drain(context.executor);

  EXPECT_FALSE(observer.events_called);
  EXPECT_TRUE(context.event_storage.events().empty());
  EXPECT_FALSE(context.fetcher->IsAlerting(node_id));
}

namespace {

const scada::EventFilter& SubscribedFilter(const TestContext& context,
                                           std::size_t index) {
  return std::get<scada::EventFilter>(
      context.monitored_item_service.params.at(index).filter);
}

scada::Event MakeConditionEvent(scada::EventId event_id,
                                const scada::NodeId& condition_id,
                                bool acked) {
  scada::Event event = MakeEvent(event_id, scada::NodeId{1, 100});
  event.event_type_id = scada::NodeId{2001, 5};
  event.condition_id = condition_id;
  event.foreign_event_id = scada::ByteString(16, static_cast<char>(event_id));
  event.acked = acked;
  return event;
}

}  // namespace

// Our server keeps its own request: OfType SystemEventType, no named fields.
TEST(EventFetcherTest, OurServerSubscriptionIsUnchanged) {
  TestContext context;
  TestEventObserver observer;
  context.StartFetcher(observer);

  const scada::EventFilter& filter = SubscribedFilter(context, 0);
  EXPECT_EQ(filter.of_type, std::vector<scada::NodeId>{
                                scada::NodeId{scada::id::SystemEventType}});
  EXPECT_EQ(filter.fields, scada::EventFilter::Fields::kScada);
}

// A third-party server's alarms are ConditionType subtypes, not
// SystemEventType ones, and it has no SCADA fields (backlog 1055).
TEST(EventFetcherTest, ForeignServerSubscriptionAsksForStandardFields) {
  TestContext context;
  TestEventObserver observer;
  context.StartFetcher(observer, /*foreign_server=*/true);

  const scada::EventFilter& filter = SubscribedFilter(context, 0);
  EXPECT_TRUE(filter.of_type.empty());
  EXPECT_EQ(filter.fields, scada::EventFilter::Fields::kStandardWithConditions);
}

// open62541 without A&C refuses the Condition fields outright; the fetcher
// then asks again for the standard fields alone.
TEST(EventFetcherTest, ForeignServerRefusingConditionFieldsFallsBack) {
  TestContext context;
  TestEventObserver observer;
  context.StartFetcher(observer, /*foreign_server=*/true);

  context.system_events_item().NotifyEvent({},
                                           scada::StatusCode::Bad_WrongNodeId);
  Drain(context.executor);

  ASSERT_EQ(context.monitored_item_service.items.size(), 2u);
  EXPECT_EQ(SubscribedFilter(context, 1).fields,
            scada::EventFilter::Fields::kStandard);

  context.monitored_item_service.items.at(1)->NotifyEvent(
      std::any{MakeEvent(42, scada::NodeId{1, 100})});
  Drain(context.executor);
  EXPECT_EQ(observer.event_count, 1u);
}

// Every state change of a Condition comes with a new EventId (OPC UA Part 9
// §5.5.2); the journal holds one entry per Condition, and drops it when the
// server reports it acknowledged.
TEST(EventFetcherTest, ForeignConditionStatesShareOneJournalEntry) {
  TestContext context;
  TestEventObserver observer;
  context.StartFetcher(observer, /*foreign_server=*/true);
  const scada::NodeId condition{scada::String{"HighTemperature"}, 5};

  context.system_events_item().NotifyEvent(
      std::any{MakeConditionEvent(1001, condition, false)});
  context.system_events_item().NotifyEvent(
      std::any{MakeConditionEvent(1002, condition, false)});
  Drain(context.executor);

  ASSERT_EQ(context.event_storage.events().size(), 1u);
  const scada::Event& stored = context.event_storage.events().begin()->second;
  EXPECT_EQ(stored.event_id, 1001u);
  // The latest notification's EventId is the one to acknowledge.
  EXPECT_EQ(stored.foreign_event_id,
            scada::ByteString(16, static_cast<char>(1002)));

  context.system_events_item().NotifyEvent(
      std::any{MakeConditionEvent(1003, condition, true)});
  Drain(context.executor);
  EXPECT_TRUE(context.event_storage.events().empty());
}

// Acknowledge goes to the Condition with its latest EventId and a Comment
// (OPC UA Part 9 §5.7.3), never as our batch call on Server.
TEST(EventFetcherTest, ForeignConditionIsAcknowledgedOnTheCondition) {
  TestContext context;
  TestEventObserver observer;
  context.StartFetcher(observer, /*foreign_server=*/true);
  const scada::NodeId condition{scada::String{"HighTemperature"}, 5};

  context.system_events_item().NotifyEvent(
      std::any{MakeConditionEvent(1001, condition, false)});
  context.system_events_item().NotifyEvent(
      std::any{MakeConditionEvent(1002, condition, false)});
  Drain(context.executor);

  context.fetcher->AcknowledgeEvent(1001);
  Drain(context.executor);

  ASSERT_EQ(context.method_service.calls.size(), 1u);
  const auto& call = context.method_service.calls[0];
  EXPECT_EQ(call.node_id, condition);
  EXPECT_EQ(call.method_id,
            scada::NodeId{scada::id::AcknowledgeableConditionType_Acknowledge});
  ASSERT_EQ(call.arguments.size(), 2u);
  EXPECT_EQ(call.arguments[0],
            scada::Variant{scada::ByteString(16, static_cast<char>(1002))});
  EXPECT_TRUE(call.arguments[1].get_if<scada::LocalizedText>());
  EXPECT_TRUE(context.event_storage.events().empty());
  EXPECT_FALSE(context.fetcher->IsAcking());
}

// A plain event has no Condition to call; it is acknowledged on this side.
TEST(EventFetcherTest, PlainForeignEventIsAcknowledgedLocally) {
  TestContext context;
  TestEventObserver observer;
  context.StartFetcher(observer, /*foreign_server=*/true);

  context.system_events_item().NotifyEvent(
      std::any{MakeEvent(42, scada::NodeId{1, 100})});
  Drain(context.executor);
  ASSERT_EQ(context.event_storage.events().size(), 1u);

  context.fetcher->AcknowledgeEvent(42);
  Drain(context.executor);

  EXPECT_TRUE(context.method_service.calls.empty());
  EXPECT_TRUE(context.event_storage.events().empty());
  EXPECT_FALSE(context.fetcher->IsAcking());
}
