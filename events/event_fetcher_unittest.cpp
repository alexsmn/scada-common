#include "events/event_fetcher.h"

#include "base/boost_log.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "events/event_ack_queue.h"
#include "events/event_observer.h"
#include "events/event_storage.h"
#include "scada/history_service.h"
#include "scada/item_factory_subscription.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
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
               const scada::MonitoringParameters& /*params*/) {
          auto item = std::make_shared<scada::TestMonitoredItem>();
          items.push_back(item);
          return item;
        },
        options);
  }

  std::vector<std::shared_ptr<scada::TestMonitoredItem>> items;
};

// HistoryService that reports no unacked history. `EventFetcher` only reads
// history from `OnChannelOpened`, which these tests do not exercise.
class FakeHistoryService final : public scada::HistoryService {
 public:
  Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails /*details*/) override {
    co_return scada::HistoryReadRawResult{.status = scada::StatusCode::Good};
  }

  Awaitable<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId /*node_id*/,
      scada::base::Time /*from*/,
      scada::base::Time /*to*/,
      scada::EventFilter /*filter*/) override {
    co_return scada::HistoryReadEventsResult{.status = scada::StatusCode::Good};
  }
};

// MethodService that accepts any call. The ack queue holds a reference to it,
// but these tests never acknowledge, so `Call` is never invoked.
class FakeMethodService final : public scada::MethodService {
 public:
  Awaitable<scada::Status> Call(scada::NodeId /*node_id*/,
                                scada::NodeId /*method_id*/,
                                std::vector<scada::Variant> /*arguments*/,
                                scada::ServiceContext /*context*/) override {
    co_return scada::Status{scada::StatusCode::Good};
  }
};

scada::Event MakeEvent(scada::EventId event_id,
                       const scada::NodeId& node_id,
                       scada::EventSeverity severity = scada::kSeverityNormal) {
  scada::Event event;
  event.event_id = event_id;
  event.node_id = node_id;
  event.severity = severity;
  return event;
}

struct TestContext {
  TestContext()
      : ack_queue{EventAckQueueContext{.logger_ = std::make_shared<BoostLogger>(LOG_NAME("Test")),
                                       .executor_ = executor,
                                       .method_service_ = method_service}} {}

  // Constructs the fetcher and drives it until its system-events monitored item
  // is established. `EventFetcher` subscribes lazily through the real
  // `CreateSubscription` path on the executor, so a drain is needed before the
  // fake item exists.
  void StartFetcher(EventObserver& observer) {
    fetcher.emplace(
        EventFetcherContext{.executor_ = executor,
                            .monitored_item_service_ = monitored_item_service,
                            .history_service_ = history_service,
                            .logger_ = std::make_shared<BoostLogger>(LOG_NAME("Test")),
                            .event_storage_ = event_storage,
                            .event_ack_queue_ = ack_queue});
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
