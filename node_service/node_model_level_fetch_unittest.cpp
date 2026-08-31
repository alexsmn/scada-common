#include "address_space/test/test_address_space.h"
#include "base/async_completion.h"
#include "base/test/test_executor.h"
#include "node_service/node_ref.h"
#include "node_service/v3/node_fetcher.h"
#include "node_service/v3/node_service_impl.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/standard_node_ids.h"
#include "scada/standard_reference_types.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace v3 {
namespace {

using testing::NiceMock;

// A fetcher that holds every node fetch open until the test releases it, and
// records how many were open at once.
//
// Concurrency is the observable here, not elapsed time: the defect this pins is
// that a level's children were fetched one at a time, and a stopwatch would
// only say so on a slow link. Holding the fetches open makes the difference
// exact and deterministic -- serially, exactly one can ever be outstanding.
class GatedNodeFetcher final : public NodeFetcher {
 public:
  GatedNodeFetcher(TestAddressSpace& address_space, AnyExecutor executor)
      : address_space_{address_space}, executor_{std::move(executor)} {}

  scada::CoStatusOr<scada::NodeState> FetchNode(
      const scada::NodeId& node_id) override {
    // The parent's own fetch is not part of the level, and it is still
    // unwinding when the level starts -- counting it would put the high-water
    // mark one above the window and make the bound assertion read as if it
    // were off by one.
    const bool counts = node_id != uncounted_;
    if (counts)
      ++in_flight_;
    if (counts)
      max_in_flight_ = std::max(max_in_flight_, in_flight_);

    {
      auto& completion = pending_.emplace_back(executor_);
      co_await completion.Wait();
    }

    if (counts)
      --in_flight_;

    const auto* node = address_space_.GetNode(node_id);
    if (!node)
      co_return scada::StatusCode::Bad_WrongNodeId;
    co_return scada::MakeNodeState(*node);
  }

  scada::CoStatusOr<scada::ReferenceDescriptions> FetchChildren(
      const scada::NodeId& node_id) override {
    const auto* node = address_space_.GetNode(node_id);
    if (!node)
      co_return scada::StatusCode::Bad_WrongNodeId;

    scada::ReferenceDescriptions references;
    for (const auto& reference : node->forward_references()) {
      if (!scada::IsSubtypeOf(*reference.type,
                              scada::id::HierarchicalReferences))
        continue;
      references.push_back({.reference_type_id = reference.type->id(),
                            .forward = true,
                            .node_id = reference.node->id(),
                            .node_class = reference.node->GetNodeClass(),
                            .browse_name = reference.node->GetBrowseName(),
                            .display_name = reference.node->GetDisplayName()});
    }
    co_return references;
  }

  // Releases the fetches held right now. Later ones are held again, so a test
  // can step through one phase of the fetch at a time.
  void ReleasePending() {
    auto pending = std::move(pending_);
    pending_.clear();
    for (const auto& completion : pending)
      completion.Complete();
  }

  int max_in_flight() const { return max_in_flight_; }

  // Excludes `node_id` from the in-flight count.
  void DoNotCount(const scada::NodeId& node_id) { uncounted_ = node_id; }

 private:
  TestAddressSpace& address_space_;
  AnyExecutor executor_;
  scada::NodeId uncounted_;
  int in_flight_ = 0;
  int max_in_flight_ = 0;
  std::vector<scada::base::AsyncCompletion> pending_;
};

class NodeModelLevelFetchTest : public testing::Test {
 protected:
  // Hangs `count` children off RootFolder and returns the parent's id. Plain
  // objects: this test is about how many fetches are in flight, not what they
  // return.
  scada::NodeId MakeFolderWithChildren(int count) {
    const scada::NodeId parent = scada::id::RootFolder;
    for (int i = 0; i < count; ++i) {
      const scada::NodeId child{static_cast<unsigned>(2000 + i),
                                TestAddressSpace::kNamespaceIndex};
      const std::string name = "Child" + std::to_string(i);
      const std::u16string display{name.begin(), name.end()};
      address_space_->CreateNode(
          scada::NodeState{.node_id = child,
                           .node_class = scada::NodeClass::Object,
                           .type_definition_id = scada::id::BaseObjectType,
                           .parent_id = parent,
                           .reference_type_id = scada::id::Organizes,
                           .attributes = scada::NodeAttributes{
                               .browse_name = scada::QualifiedName{name},
                               .display_name = scada::LocalizedText{display}}});
    }
    return parent;
  }

  void Drain() {
    for (int i = 0; i < 200 && executor_.GetTaskCount() != 0; ++i)
      executor_.Poll();
  }

  // Runs the parent's own node fetch, leaving the level's child fetches held
  // and the in-flight high-water mark measuring only them.
  void StartLevelFetch(const scada::NodeId& parent) {
    fetcher_->DoNotCount(parent);
    node_service_.OnChannelOpened();
    Drain();
    node_service_.GetNode(parent).StartFetch(NodeFetchStatus::NodeAndChildren);
    Drain();
    // Releasing the parent's own fetch lets FetchChildren run, which issues
    // the level. Those are held in turn, so the high-water mark below is the
    // level's -- the parent contributed only 1, which every assertion clears.
    fetcher_->ReleasePending();
    Drain();
  }

  TestExecutor executor_;
  std::shared_ptr<TestAddressSpace> address_space_{
      std::make_shared<TestAddressSpace>()};
  std::shared_ptr<GatedNodeFetcher> fetcher_{
      std::make_shared<GatedNodeFetcher>(*address_space_, executor_)};
  std::shared_ptr<NodeFetcher> fetcher_base_{fetcher_};
  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  NodeServiceImpl node_service_{
      NodeServiceImplContext{.executor_ = executor_,
                             .monitored_item_service_ = monitored_item_service_,
                             .node_fetcher_ = fetcher_base_,
                             .keep_alive_capacity_ = 1024}};
};

// The defect: a folder's children were awaited one at a time, so a level of N
// cost N sequential round trips before any of it could be published. With every
// fetch held open, the serial form can never have more than one outstanding.
TEST_F(NodeModelLevelFetchTest, ChildrenOfALevelAreFetchedConcurrently) {
  const scada::NodeId parent = MakeFolderWithChildren(8);

  StartLevelFetch(parent);

  // Every child is outstanding at once. Pre-fix this is 1.
  EXPECT_GT(fetcher_->max_in_flight(), 1);

  for (int i = 0; i < 20; ++i) {
    fetcher_->ReleasePending();
    Drain();
  }

  EXPECT_TRUE(node_service_.GetNode(parent).children_fetched());
}

// The window is a bound, not a suggestion: a level wider than it must not put
// one in-flight request per child on the session.
TEST_F(NodeModelLevelFetchTest, ConcurrencyIsBounded) {
  const scada::NodeId parent = MakeFolderWithChildren(64);

  StartLevelFetch(parent);

  EXPECT_GT(fetcher_->max_in_flight(), 1);
  EXPECT_LE(fetcher_->max_in_flight(), 16);

  for (int i = 0; i < 20; ++i) {
    fetcher_->ReleasePending();
    Drain();
  }

  EXPECT_TRUE(node_service_.GetNode(parent).children_fetched());
}

}  // namespace
}  // namespace v3
