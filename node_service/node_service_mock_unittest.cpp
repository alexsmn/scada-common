#include "node_service/node_service_mock.h"

#include "base/test/awaitable_test.h"
#include "node_service/node_fetch_status.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

// An unstubbed `Fetch` must hand back an awaitable that can actually be
// awaited. gmock's fallback for a return type it knows nothing about is `T()`,
// and a default-constructed `boost::asio::awaitable` holds a null frame whose
// `await_ready()` still reports false -- so co_awaiting it dereferences null in
// `await_suspend` and segfaults inside the *awaiting* coroutine, with no
// mention of the mock anywhere in the crash. The command palette's tag browse
// died exactly that way. A `NiceMock` is the case that matters: a `StrictMock`
// would flag the unexpected call first.
TEST(NodeServiceMockTest, UnstubbedFetchYieldsAnAwaitableThatCompletes) {
  TestExecutor executor;
  NiceMock<MockNodeService> node_service;

  bool completed = false;
  WaitAwaitable(executor, [&]() -> Awaitable<void> {
    co_await node_service.Fetch(scada::NodeId{1},
                                NodeFetchStatus::NodeAndChildren);
    completed = true;
  }());

  EXPECT_TRUE(completed);
}

}  // namespace
