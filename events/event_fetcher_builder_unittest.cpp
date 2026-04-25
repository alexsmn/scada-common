#include "events/event_fetcher_builder.h"

#include "base/logger.h"
#include "base/test/test_executor.h"
#include "events/event_fetcher.h"
#include "scada/coroutine_services.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/status.h"
#include "scada/status_exception.h"

#include <boost/signals2/signal.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using testing::_;
using testing::NiceMock;
using testing::VariantWith;

void DrainExecutor(const std::shared_ptr<TestExecutor>& executor) {
  for (size_t i = 0; i < 100 && executor->GetTaskCount() != 0; ++i)
    executor->Poll();
}

class TestCoroutineHistoryService final
    : public scada::CoroutineHistoryService {
 public:
  Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override {
    co_return scada::HistoryReadRawResult{.status = scada::StatusCode::Bad};
  }

  Awaitable<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId node_id,
      base::Time from,
      base::Time to,
      scada::EventFilter filter) override {
    ++read_events_count;
    last_filter = filter;
    co_return result;
  }

  int read_events_count = 0;
  scada::EventFilter last_filter;
  scada::HistoryReadEventsResult result;
};

class TestCoroutineMethodService final : public scada::CoroutineMethodService {
 public:
  Awaitable<scada::Status> Call(scada::NodeId node_id,
                                scada::NodeId method_id,
                                std::vector<scada::Variant> arguments,
                                scada::NodeId user_id) override {
    ++call_count;
    last_node_id = std::move(node_id);
    last_method_id = std::move(method_id);
    last_arguments = std::move(arguments);
    last_user_id = std::move(user_id);
    co_return scada::Status{scada::StatusCode::Good};
  }

  int call_count = 0;
  scada::NodeId last_node_id;
  scada::NodeId last_method_id;
  std::vector<scada::Variant> last_arguments;
  scada::NodeId last_user_id;
};

class TestCoroutineSessionService final : public scada::CoroutineSessionService {
 public:
  Awaitable<void> Connect(scada::SessionConnectParams params) override {
    co_return;
  }

  Awaitable<void> Reconnect() override { co_return; }

  Awaitable<void> Disconnect() override { co_return; }

  bool IsConnected(base::TimeDelta* ping_delay = nullptr) const override {
    return connected;
  }

  scada::NodeId GetUserId() const override { return user_id; }

  bool HasPrivilege(scada::Privilege privilege) const override { return true; }

  std::string GetHostName() const override { return "test"; }

  bool IsScada() const override { return true; }

  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override {
    return session_state_changed.connect(callback);
  }

  scada::SessionDebugger* GetSessionDebugger() override { return nullptr; }

  bool connected = true;
  scada::NodeId user_id{700, 5};
  boost::signals2::signal<void(bool, const scada::Status&)>
      session_state_changed;
};

scada::Event MakeEvent(scada::EventId event_id, scada::NodeId node_id) {
  scada::Event event;
  event.event_id = event_id;
  event.node_id = std::move(node_id);
  event.severity = scada::kSeverityMax;
  return event;
}

}  // namespace

TEST(CoroutineEventFetcherBuilder,
     ConnectedSessionRefreshesHistoryThroughCoroutineServices) {
  const auto executor = std::make_shared<TestExecutor>();
  NiceMock<scada::MockMonitoredItemService> monitored_item_service;
  TestCoroutineHistoryService history_service;
  TestCoroutineMethodService method_service;
  TestCoroutineSessionService session_service;
  const scada::NodeId node_id{42, 100};
  history_service.result.events = {MakeEvent(12, node_id)};

  EXPECT_CALL(*monitored_item_service.default_monitored_item,
              Subscribe(VariantWith<scada::EventHandler>(_)));

  auto fetcher = CoroutineEventFetcherBuilder{
      .executor_ = MakeTestAnyExecutor(executor),
      .logger_ = NullLogger::GetInstance(),
      .monitored_item_service_ = monitored_item_service,
      .history_service_ = history_service,
      .method_service_ = method_service,
      .session_service_ = session_service}
                     .Build();

  DrainExecutor(executor);

  EXPECT_EQ(history_service.read_events_count, 1);
  EXPECT_TRUE(fetcher->IsAlerting(node_id));
}

TEST(CoroutineEventFetcherBuilder,
     AcknowledgeUsesCoroutineMethodServiceAndSessionUser) {
  const auto executor = std::make_shared<TestExecutor>();
  NiceMock<scada::MockMonitoredItemService> monitored_item_service;
  TestCoroutineHistoryService history_service;
  TestCoroutineMethodService method_service;
  TestCoroutineSessionService session_service;
  const scada::NodeId node_id{43, 100};
  history_service.result.events = {MakeEvent(13, node_id)};

  EXPECT_CALL(*monitored_item_service.default_monitored_item,
              Subscribe(VariantWith<scada::EventHandler>(_)));

  auto fetcher = CoroutineEventFetcherBuilder{
      .executor_ = MakeTestAnyExecutor(executor),
      .logger_ = NullLogger::GetInstance(),
      .monitored_item_service_ = monitored_item_service,
      .history_service_ = history_service,
      .method_service_ = method_service,
      .session_service_ = session_service}
                     .Build();
  DrainExecutor(executor);

  fetcher->AcknowledgeEvent(13);
  DrainExecutor(executor);

  EXPECT_EQ(method_service.call_count, 1);
  EXPECT_EQ(method_service.last_user_id, session_service.user_id);
  ASSERT_EQ(method_service.last_arguments.size(), 2u);
}
