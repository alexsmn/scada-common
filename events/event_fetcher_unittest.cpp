#include "events/event_fetcher.h"

#include "base/logger.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "events/event_ack_queue.h"
#include "events/event_observer.h"
#include "events/event_storage.h"
#include "scada/coroutine_services.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/status.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>

namespace {

using testing::_;
using testing::StrictMock;
using testing::VariantWith;

class TestEventObserver final : public EventObserver {
 public:
  void OnEvents(std::span<const scada::Event* const> events) override {
    events_called = true;
    event_count += events.size();
  }

  bool events_called = false;
  std::size_t event_count = 0;
};

struct TestContext {
  TestContext()
      : ack_queue{EventAckQueueContext{
            .logger_ = NullLogger::GetInstance(),
            .executor_ = MakeTestAnyExecutor(executor),
            .method_service_ = method_service_adapter}} {}

  std::shared_ptr<TestExecutor> executor = std::make_shared<TestExecutor>();
  StrictMock<scada::MockMonitoredItemService> monitored_item_service;
  StrictMock<scada::MockHistoryService> history_service;
  scada::CallbackToCoroutineHistoryServiceAdapter history_service_adapter{
      MakeTestAnyExecutor(executor), history_service};
  StrictMock<scada::MockMethodService> method_service;
  scada::CallbackToCoroutineMethodServiceAdapter method_service_adapter{
      MakeTestAnyExecutor(executor), method_service};
  EventStorage event_storage;
  EventAckQueue ack_queue;
  std::optional<EventFetcher> fetcher;
  scada::EventHandler event_handler;
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

}  // namespace

TEST(EventFetcherTest, MonitoredItemEventsAreDeliveredThroughCoroutineTask) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  TestEventObserver observer;

  EXPECT_CALL(*context.monitored_item_service.default_monitored_item,
              Subscribe(VariantWith<scada::EventHandler>(_)))
      .WillOnce([&](scada::MonitoredItemHandler handler) {
        context.event_handler =
            std::move(std::get<scada::EventHandler>(handler));
      });
  EXPECT_CALL(context.monitored_item_service, CreateMonitoredItem(_, _))
      .WillOnce(testing::Return(
          context.monitored_item_service.default_monitored_item));

  context.fetcher.emplace(EventFetcherContext{
      .executor_ = MakeTestAnyExecutor(context.executor),
      .monitored_item_service_ = context.monitored_item_service,
      .history_service_ = context.history_service_adapter,
      .logger_ = NullLogger::GetInstance(),
      .event_storage_ = context.event_storage,
      .event_ack_queue_ = context.ack_queue});
  context.fetcher->AddObserver(observer);

  context.event_handler(scada::Status{scada::StatusCode::Good},
                        MakeEvent(42, node_id));

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

  EXPECT_CALL(*context.monitored_item_service.default_monitored_item,
              Subscribe(VariantWith<scada::EventHandler>(_)))
      .WillOnce([&](scada::MonitoredItemHandler handler) {
        context.event_handler =
            std::move(std::get<scada::EventHandler>(handler));
      });
  EXPECT_CALL(context.monitored_item_service, CreateMonitoredItem(_, _))
      .WillOnce(testing::Return(
          context.monitored_item_service.default_monitored_item));

  context.fetcher.emplace(EventFetcherContext{
      .executor_ = MakeTestAnyExecutor(context.executor),
      .monitored_item_service_ = context.monitored_item_service,
      .history_service_ = context.history_service_adapter,
      .logger_ = NullLogger::GetInstance(),
      .event_storage_ = context.event_storage,
      .event_ack_queue_ = context.ack_queue});
  context.fetcher->AddObserver(observer);
  context.fetcher.reset();

  context.event_handler(scada::Status{scada::StatusCode::Good},
                        MakeEvent(42, node_id));
  Drain(context.executor);

  EXPECT_FALSE(observer.events_called);
  EXPECT_TRUE(context.event_storage.events().empty());
}

TEST(EventFetcherTest, MonitoredItemEventWithBadStatusIsIgnored) {
  TestContext context;
  const scada::NodeId node_id{1, 100};
  TestEventObserver observer;

  EXPECT_CALL(*context.monitored_item_service.default_monitored_item,
              Subscribe(VariantWith<scada::EventHandler>(_)))
      .WillOnce([&](scada::MonitoredItemHandler handler) {
        context.event_handler =
            std::move(std::get<scada::EventHandler>(handler));
      });
  EXPECT_CALL(context.monitored_item_service, CreateMonitoredItem(_, _))
      .WillOnce(testing::Return(
          context.monitored_item_service.default_monitored_item));

  context.fetcher.emplace(EventFetcherContext{
      .executor_ = MakeTestAnyExecutor(context.executor),
      .monitored_item_service_ = context.monitored_item_service,
      .history_service_ = context.history_service_adapter,
      .logger_ = NullLogger::GetInstance(),
      .event_storage_ = context.event_storage,
      .event_ack_queue_ = context.ack_queue});
  context.fetcher->AddObserver(observer);

  context.event_handler(scada::Status{scada::StatusCode::Bad},
                        MakeEvent(42, node_id));
  Drain(context.executor);

  EXPECT_FALSE(observer.events_called);
  EXPECT_TRUE(context.event_storage.events().empty());
  EXPECT_FALSE(context.fetcher->IsAlerting(node_id));
}
