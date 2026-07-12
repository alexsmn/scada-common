// Regression coverage for already-satisfied NodeRef coroutine fetches.

#include "node_service/node_ref.h"
#include "node_service/test/fake_node_service.h"

#include "base/awaitable.h"
#include "common/node_state.h"
#include "scada/standard_node_ids.h"

#include <boost/asio/io_context.hpp>

#include <gtest/gtest.h>

namespace {

scada::NodeId Id(scada::NumericId numeric_id) {
  return scada::NodeId{numeric_id};
}

}  // namespace

TEST(NodeRefFetchRecursion, SameNodeCoroutineFetchCanRepeat) {
  FakeNodeService service;
  NodeRef node = service.Add(scada::NodeState{}.set_node_id(Id(1)));

  constexpr int kRepeat = 2000;

  int fetch_count = 0;
  boost::asio::io_context io_context;
  RunAwaitable(io_context, [&]() -> Awaitable<void> {
    for (int i = 0; i < kRepeat; ++i) {
      co_await node.Fetch(NodeFetchStatus::NodeOnly);
      ++fetch_count;
    }
  });

  EXPECT_EQ(fetch_count, kRepeat);
}

TEST(NodeRefFetchRecursion, CrossNodeCoroutineFetchCanWalkChain) {
  constexpr int kChainLength = 64;

  FakeNodeService service;
  // Link node i to node i+1 with an Organizes reference so the walk below can
  // step through the chain via NodeRef navigation.
  for (int i = 0; i < kChainLength; ++i) {
    scada::NodeState state;
    state.set_node_id(Id(i + 1));
    if (i + 1 < kChainLength) {
      state.add_reference(scada::ReferenceDescription{scada::id::Organizes,
                                                      true, Id(i + 2)});
    }
    service.Add(std::move(state));
  }

  int fetch_count = 0;
  boost::asio::io_context io_context;
  RunAwaitable(io_context, [&]() -> Awaitable<void> {
    for (NodeRef node = service.GetNode(Id(1)); node;) {
      co_await node.Fetch(NodeFetchStatus::NodeOnly);
      ++fetch_count;
      node = node.target(scada::id::Organizes);
    }
  });

  EXPECT_EQ(fetch_count, kChainLength);
}
