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
        .executor_ = MakeTestAnyExecutor(executor_),
        .method_service_ = method_service_adapter_}};
  }

  void DrainExecutor() { Drain(executor_); }

  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  StrictMock<scada::MockMethodService> method_service_;
  scada::CallbackToCoroutineMethodServiceAdapter method_service_adapter_{
      MakeTestAnyExecutor(executor_), method_service_};
};

MATCHER_P(ArgumentsContainEventIds, event_ids, "") {
  std::vector<scada::EventId> actual;
  if (arg.empty() || !arg[0].get(actual))
    return false;
  return actual == event_ids;
}

}  // namespace

TEST_F(EventAckQueueTest, AckDispatchesMethodCallFromCoroutineTask) {
  auto queue = MakeQueue();
  queue.OnChannelOpened(scada::id::Server);

  EXPECT_CALL(method_service_,
              Call(scada::NodeId{scada::id::Server},
                   scada::NodeId{
                       scada::id::AcknowledgeableConditionType_Acknowledge},
                   ArgumentsContainEventIds(std::vector<scada::EventId>{11}),
                   scada::NodeId{scada::id::Server}, _));

  queue.Ack(11);
  DrainExecutor();
}

TEST_F(EventAckQueueTest, AckAcceptsDelayedMethodCompletionThroughAdapter) {
  auto queue = MakeQueue();
  queue.OnChannelOpened(scada::id::Server);
  scada::StatusCallback callback;

  EXPECT_CALL(method_service_,
              Call(_, _,
                   ArgumentsContainEventIds(std::vector<scada::EventId>{11}),
                   _, _))
      .WillOnce(SaveArg<4>(&callback));

  queue.Ack(11);
  DrainExecutor();
  ASSERT_TRUE(callback);

  callback(scada::StatusCode::Good);
  DrainExecutor();
}

TEST_F(EventAckQueueTest, AckKeepsAtMostFiveRunningAndSchedulesRemainder) {
  auto queue = MakeQueue();
  queue.OnChannelOpened(scada::id::Server);

  EXPECT_CALL(method_service_,
              Call(_, _, ArgumentsContainEventIds(
                             std::vector<scada::EventId>{1, 2, 3, 4, 5}),
                   _, _));

  for (scada::EventId event_id = 1; event_id <= 6; ++event_id)
    queue.Ack(event_id);
  DrainExecutor();

  EXPECT_TRUE(queue.IsAcking());

  EXPECT_CALL(method_service_,
              Call(_, _,
                   ArgumentsContainEventIds(std::vector<scada::EventId>{6}),
                   _, _));

  queue.OnAcked(1);
  DrainExecutor();
}

TEST_F(EventAckQueueTest, DuplicateAckIsIgnoredWhilePending) {
  auto queue = MakeQueue();
  queue.OnChannelOpened(scada::id::Server);

  EXPECT_CALL(method_service_,
              Call(_, _,
                   ArgumentsContainEventIds(std::vector<scada::EventId>{42}),
                   _, _));

  queue.Ack(42);
  queue.Ack(42);
  DrainExecutor();
}
