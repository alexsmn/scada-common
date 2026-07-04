#include "events/event_ack_queue.h"

#include "base/logger.h"
#include "base/test/awaitable_test.h"
#include "scada/coroutine_services.h"
#include "scada/method_service_mock.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

namespace {

class EventAckQueueTest : public Test {
 protected:
  EventAckQueue MakeQueue() {
    return EventAckQueue{EventAckQueueContext{
        .logger_ = NullLogger::GetInstance(),
        .executor_ = executor_,
        .method_service_ = method_service_}};
  }

  void DrainExecutor() { Drain(executor_); }

  TestExecutor executor_;
  StrictMock<scada::MockMethodService> method_service_;
};

Awaitable<scada::Status> MakeStatusAwaitable(
    scada::Status status = scada::StatusCode::Good) {
  co_return std::move(status);
}

MATCHER_P(ArgumentsContainEventIds, event_ids, "") {
  std::vector<scada::EventId> actual;
  if (arg.empty() || !arg[0].get(actual))
    return false;
  return actual == event_ids;
}

MATCHER_P(ContextHasUserId, user_id, "") {
  return arg.user_id() == user_id;
}

}  // namespace

TEST_F(EventAckQueueTest, AckDispatchesMethodCallFromCoroutineTask) {
  auto queue = MakeQueue();
  const scada::NodeId user_id{scada::id::Server};
  queue.OnChannelOpened(scada::ServiceContext{}.with_user_id(user_id));

  EXPECT_CALL(method_service_,
              Call(scada::NodeId{scada::id::Server},
                   scada::NodeId{
                       scada::id::AcknowledgeableConditionType_Acknowledge},
                   ArgumentsContainEventIds(std::vector<scada::EventId>{11}),
                   ContextHasUserId(user_id)))
      .WillOnce(Invoke([](auto, auto, auto, auto) {
        return MakeStatusAwaitable();
      }));

  queue.Ack(11);
  DrainExecutor();
}

TEST_F(EventAckQueueTest, AckKeepsAtMostFiveRunningAndSchedulesRemainder) {
  auto queue = MakeQueue();
  queue.OnChannelOpened(
      scada::ServiceContext{}.with_user_id(scada::NodeId{scada::id::Server}));

  EXPECT_CALL(method_service_,
              Call(_, _, ArgumentsContainEventIds(
                             std::vector<scada::EventId>{1, 2, 3, 4, 5}),
                   _))
      .WillOnce(Invoke([](auto, auto, auto, auto) {
        return MakeStatusAwaitable();
      }));

  for (scada::EventId event_id = 1; event_id <= 6; ++event_id)
    queue.Ack(event_id);
  DrainExecutor();

  EXPECT_TRUE(queue.IsAcking());

  EXPECT_CALL(method_service_,
              Call(_, _,
                   ArgumentsContainEventIds(std::vector<scada::EventId>{6}),
                   _))
      .WillOnce(Invoke([](auto, auto, auto, auto) {
        return MakeStatusAwaitable();
      }));

  queue.OnAcked(1);
  DrainExecutor();
}

TEST_F(EventAckQueueTest, DuplicateAckIsIgnoredWhilePending) {
  auto queue = MakeQueue();
  queue.OnChannelOpened(
      scada::ServiceContext{}.with_user_id(scada::NodeId{scada::id::Server}));

  EXPECT_CALL(method_service_,
              Call(_, _,
                   ArgumentsContainEventIds(std::vector<scada::EventId>{42}),
                   _))
      .WillOnce(Invoke([](auto, auto, auto, auto) {
        return MakeStatusAwaitable();
      }));

  queue.Ack(42);
  queue.Ack(42);
  DrainExecutor();
}

TEST_F(EventAckQueueTest, DestroyedQueueSuppressesPendingAckDispatch) {
  {
    auto queue = MakeQueue();
    queue.OnChannelOpened(
      scada::ServiceContext{}.with_user_id(scada::NodeId{scada::id::Server}));
    queue.Ack(11);
  }

  DrainExecutor();
}
