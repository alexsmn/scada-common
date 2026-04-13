#include "node_service/node_promises.h"

#include "node_service/node_observer.h"
#include "node_service/node_service.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace {

class FakeNodeService final : public NodeService {
 public:
  NodeRef GetNode(const scada::NodeId&) override { return {}; }

  void Subscribe(NodeRefObserver& observer) const override {
    observers_.push_back(&observer);
    ++subscribe_count_;
  }

  void Unsubscribe(NodeRefObserver& observer) const override {
    std::erase(observers_, &observer);
    ++unsubscribe_count_;
  }

  size_t GetPendingTaskCount() const override { return pending_task_count_; }

  void SetPendingTaskCount(size_t pending_task_count) {
    pending_task_count_ = pending_task_count;
  }

  void EmitFetched() const {
    const auto observers = observers_;
    for (NodeRefObserver* observer : observers) {
      observer->OnNodeFetched(
          {{}, NodeFetchStatus::NodeOnly()});
    }
  }

  size_t subscribe_count() const { return subscribe_count_; }
  size_t unsubscribe_count() const { return unsubscribe_count_; }

 private:
  mutable std::vector<NodeRefObserver*> observers_;
  size_t pending_task_count_ = 0;
  mutable size_t subscribe_count_ = 0;
  mutable size_t unsubscribe_count_ = 0;
};

}  // namespace

TEST(NodePromisesTest, WaitForPendingNodes_ResolvesImmediatelyWhenIdle) {
  FakeNodeService node_service;

  bool resolved = false;
  std::move(WaitForPendingNodes(node_service)).then([&] { resolved = true; });

  EXPECT_TRUE(resolved);
  EXPECT_EQ(node_service.subscribe_count(), 0u);
  EXPECT_EQ(node_service.unsubscribe_count(), 0u);
}

TEST(NodePromisesTest, WaitForPendingNodes_ResolvesAfterLastFetchCompletes) {
  FakeNodeService node_service;
  node_service.SetPendingTaskCount(1);

  bool resolved = false;
  std::move(WaitForPendingNodes(node_service)).then([&] { resolved = true; });

  EXPECT_FALSE(resolved);
  EXPECT_EQ(node_service.subscribe_count(), 1u);
  EXPECT_EQ(node_service.unsubscribe_count(), 0u);

  node_service.SetPendingTaskCount(0);
  node_service.EmitFetched();

  EXPECT_TRUE(resolved);
  EXPECT_EQ(node_service.unsubscribe_count(), 1u);
}
