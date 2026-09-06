#include "events/event_notifier_mock.h"

#include "base/test/awaitable_test.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

// An unstubbed `NotifyEventAsync` completes with a bad status rather than
// with gmock's null-frame awaitable, which would segfault the awaiting
// coroutine (task 698; CLAUDE.md, "Unit Test Guidance").
TEST(EventNotifierMockTest, UnstubbedNotifyCompletesWithABadStatus) {
  TestExecutor executor;
  NiceMock<MockEventNotifier> notifier;

  scada::StatusOr<scada::EventId> id =
      WaitAwaitable(executor, notifier.NotifyEventAsync(scada::Event{}));

  EXPECT_FALSE(id.has_value());
}

}  // namespace
