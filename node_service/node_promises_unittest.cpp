#include "node_service/node_promises.h"

#include "node_service/node_model.h"
#include "node_service/node_observer.h"
#include "node_service/node_service.h"
#include "node_service/v3/node_fetcher.h"
#include "scada/status_exception.h"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <future>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

class AwaitableTestRunner {
 public:
  template <class T>
  T Wait(Awaitable<T> awaitable) {
    auto future = Start(std::move(awaitable));
    RunUntilReady(future);
    return future.get();
  }

  void Wait(Awaitable<void> awaitable) {
    auto future = Start(std::move(awaitable));
    RunUntilReady(future);
    future.get();
  }

  template <class T>
  std::future<T> Start(Awaitable<T> awaitable) {
    return boost::asio::co_spawn(
        io_context_,
        [awaitable = std::move(awaitable)]() mutable -> Awaitable<T> {
          if constexpr (std::is_void_v<T>) {
            co_await std::move(awaitable);
            co_return;
          } else {
            co_return co_await std::move(awaitable);
          }
        },
        boost::asio::use_future);
  }

  void Poll() {
    io_context_.restart();
    while (io_context_.poll() != 0) {
    }
  }

  template <class T>
  void RunUntilReady(std::future<T>& future) {
    using namespace std::chrono_literals;
    while (future.wait_for(0ms) == std::future_status::timeout) {
      io_context_.restart();
      io_context_.run_one();
    }
  }

 private:
  boost::asio::io_context io_context_;
};

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

class TestNodeModel final : public NodeModel,
                            public std::enable_shared_from_this<TestNodeModel> {
 public:
  explicit TestNodeModel(scada::Status status = scada::StatusCode::Good)
      : status_{std::move(status)} {}

  scada::Status GetStatus() const override { return status_; }

  NodeFetchStatus GetFetchStatus() const override { return fetch_status_; }

  void Fetch(const NodeFetchStatus& requested_status,
             const FetchCallback& callback) const override {
    requested_status_ |= requested_status;
    callback_ = callback;
    if (complete_synchronously_) {
      Complete(requested_status);
    }
  }

  scada::Variant GetAttribute(scada::AttributeId attribute_id) const override {
    if (attribute_id == scada::AttributeId::NodeId) {
      return node_id_;
    }
    return {};
  }

  NodeRef GetDataType() const override { return nullptr; }

  NodeRef::Reference GetReference(const scada::NodeId& reference_type_id,
                                  bool forward,
                                  const scada::NodeId& node_id) const override {
    return {};
  }

  std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const override {
    return {};
  }

  NodeRef GetTarget(const scada::NodeId& reference_type_id,
                    bool forward) const override {
    if (forward && reference_type_id == scada::id::HasTypeDefinition) {
      return NodeRef{shared_from_this()};
    }
    return nullptr;
  }

  std::vector<NodeRef> GetTargets(const scada::NodeId& reference_type_id,
                                  bool forward) const override {
    if (!forward || reference_type_id != scada::id::HierarchicalReferences) {
      return {};
    }
    return children_;
  }

  NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const override {
    return nullptr;
  }

  NodeRef GetChild(const scada::QualifiedName& child_name) const override {
    return nullptr;
  }

  void Subscribe(NodeRefObserver& observer) const override {}
  void Unsubscribe(NodeRefObserver& observer) const override {}

  scada::node GetScadaNode() const override { return {}; }

  void Complete(NodeFetchStatus status) const {
    fetch_status_ |= status;
    auto callback = std::move(callback_);
    callback_ = nullptr;
    if (callback) {
      callback();
    }
  }

  void set_complete_synchronously(bool value) {
    complete_synchronously_ = value;
  }

  void set_status(scada::Status status) { status_ = std::move(status); }

  void add_child(NodeRef child) { children_.push_back(std::move(child)); }

  const NodeFetchStatus& requested_status() const { return requested_status_; }

  static std::shared_ptr<TestNodeModel> Create(
      scada::Status status = scada::StatusCode::Good) {
    return std::shared_ptr<TestNodeModel>(new TestNodeModel{status});
  }

 private:
  scada::Status status_;
  const scada::NodeId node_id_{1};
  mutable NodeFetchStatus fetch_status_;
  mutable NodeFetchStatus requested_status_;
  mutable FetchCallback callback_;
  bool complete_synchronously_ = false;
  std::vector<NodeRef> children_;
};

class TypeSystemNodeService final : public NodeService {
 public:
  explicit TypeSystemNodeService(NodeRef types_folder)
      : types_folder_{std::move(types_folder)} {}

  NodeRef GetNode(const scada::NodeId& node_id) override {
    return node_id == scada::id::TypesFolder ? types_folder_ : nullptr;
  }

  void Subscribe(NodeRefObserver& observer) const override {}
  void Unsubscribe(NodeRefObserver& observer) const override {}
  size_t GetPendingTaskCount() const override { return 0; }

 private:
  NodeRef types_folder_;
};

}  // namespace

TEST(NodePromisesTest, WaitForPendingNodes_ResolvesImmediatelyWhenIdle) {
  FakeNodeService node_service;
  AwaitableTestRunner runner;

  runner.Wait(WaitForPendingNodes(node_service));

  EXPECT_EQ(node_service.subscribe_count(), 0u);
  EXPECT_EQ(node_service.unsubscribe_count(), 0u);
}

TEST(NodePromisesTest, WaitForPendingNodes_ResolvesAfterLastFetchCompletes) {
  FakeNodeService node_service;
  AwaitableTestRunner runner;
  node_service.SetPendingTaskCount(1);

  auto future = runner.Start(WaitForPendingNodes(node_service));
  runner.Poll();

  EXPECT_EQ(future.wait_for(std::chrono::milliseconds{0}),
            std::future_status::timeout);
  EXPECT_EQ(node_service.subscribe_count(), 1u);
  EXPECT_EQ(node_service.unsubscribe_count(), 0u);

  node_service.SetPendingTaskCount(0);
  node_service.EmitFetched();
  runner.RunUntilReady(future);

  EXPECT_NO_THROW(future.get());
  EXPECT_EQ(node_service.unsubscribe_count(), 1u);
}

TEST(NodePromisesTest, FetchNodeAwaitsDelayedFetchCompletion) {
  AwaitableTestRunner runner;
  auto model = TestNodeModel::Create();
  NodeRef node{model};

  auto future = runner.Start(FetchNode(node));
  runner.Poll();

  EXPECT_EQ(future.wait_for(std::chrono::milliseconds{0}),
            std::future_status::timeout);
  EXPECT_TRUE(model->requested_status().node_fetched);

  model->Complete(NodeFetchStatus::NodeOnly());
  runner.RunUntilReady(future);

  EXPECT_NO_THROW(future.get());
}

TEST(NodePromisesTest, FetchChildrenThrowsStatusExceptionOnBadStatus) {
  AwaitableTestRunner runner;
  auto model = TestNodeModel::Create(scada::StatusCode::Bad_Disconnected);
  model->set_complete_synchronously(true);
  NodeRef node{model};

  EXPECT_THROW(runner.Wait(FetchChildren(node)), scada::status_exception);
}

TEST(NodePromisesTest, FetchRecursiveFetchesChildrenDepthFirst) {
  AwaitableTestRunner runner;
  auto root = TestNodeModel::Create();
  auto child = TestNodeModel::Create();
  root->set_complete_synchronously(true);
  child->set_complete_synchronously(true);
  root->add_child(NodeRef{child});

  runner.Wait(FetchRecursive(NodeRef{root}, scada::id::HierarchicalReferences));

  EXPECT_TRUE(root->requested_status().children_fetched);
  EXPECT_TRUE(child->requested_status().children_fetched);
}

TEST(NodePromisesTest, FetchTypeSystemFetchesTypesFolderRecursively) {
  AwaitableTestRunner runner;
  auto root = TestNodeModel::Create();
  auto child = TestNodeModel::Create();
  root->set_complete_synchronously(true);
  child->set_complete_synchronously(true);
  root->add_child(NodeRef{child});
  TypeSystemNodeService node_service{NodeRef{root}};

  runner.Wait(FetchTypeSystem(node_service));

  EXPECT_TRUE(root->requested_status().children_fetched);
  EXPECT_TRUE(child->requested_status().children_fetched);
}

TEST(NodePromisesTest, V3NodeFetcherReturnsCoroutineDefaults) {
  AwaitableTestRunner runner;
  v3::NodeFetcher fetcher;

  auto node_state = runner.Wait(fetcher.FetchNode(scada::NodeId{1}));
  auto children = runner.Wait(fetcher.FetchChildren(scada::NodeId{1}));

  EXPECT_TRUE(node_state.node_id.is_null());
  EXPECT_TRUE(children.empty());
}
