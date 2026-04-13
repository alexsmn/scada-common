// Regression test for the synchronous Fetch-callback recursion that
// blows the stack when the screenshot generator boots on top of
// AddressSpaceImpl3. The callback for a parent's Fetch calls Fetch on
// each child; with in-process services the child's status is already
// satisfied, so `BaseNodeModel::Fetch` invokes the child callback
// inline (see base_node_model.cpp:18–21), which calls Fetch on the
// grandchild, and so on. The chain is bounded only by tree depth or
// thread stack — whichever comes first.
//
// The test builds a chain of models whose "child" NodeRef is the next
// link in the chain. A Fetch on the root, with a callback that walks
// into the child, must not recurse past `kMaxDepth` frames — if it
// does, the stack overflows.

#include "node_service/base_node_model.h"
#include "node_service/node_ref.h"

#include <gtest/gtest.h>

#include <cassert>
#include <functional>
#include <memory>

namespace {

// Chain link: a BaseNodeModel that reports `NodeOnly` as already
// fetched (so `Fetch` fires the callback synchronously) and exposes
// the next link as its child.
class ChainNodeModel : public BaseNodeModel {
 public:
  static std::shared_ptr<ChainNodeModel> Create() {
    auto model = std::shared_ptr<ChainNodeModel>{new ChainNodeModel{}};
    // Seed the fetch status so `Fetch` takes the synchronous-callback
    // branch in `BaseNodeModel::Fetch`.
    model->SetFetchStatus(scada::StatusCode::Good, NodeFetchStatus::Max());
    return model;
  }

  void set_child(NodeRef child) { child_ = std::move(child); }
  const NodeRef& child() const { return child_; }

  // NodeModel stubs — not exercised by this test.
  scada::Variant GetAttribute(scada::AttributeId) const override { return {}; }
  NodeRef GetDataType() const override { return {}; }
  NodeRef::Reference GetReference(const scada::NodeId&,
                                  bool,
                                  const scada::NodeId&) const override {
    return {};
  }
  std::vector<NodeRef::Reference> GetReferences(const scada::NodeId&,
                                                bool) const override {
    return {};
  }
  NodeRef GetTarget(const scada::NodeId&, bool) const override { return {}; }
  std::vector<NodeRef> GetTargets(const scada::NodeId&,
                                  bool) const override {
    return {};
  }
  NodeRef GetAggregate(const scada::NodeId&) const override { return {}; }
  NodeRef GetChild(const scada::QualifiedName&) const override { return {}; }
  scada::node GetScadaNode() const override { return {}; }

 private:
  ChainNodeModel() = default;

  NodeRef child_;
};

// Walks the chain via Fetch callbacks. At every frame the callback
// calls Fetch on the child's NodeRef, passing itself as the next
// callback — pure synchronous recursion under the sync-callback
// branch of BaseNodeModel::Fetch.
struct RecursiveWalker {
  int depth = 0;
  int max_depth = 0;

  // Upper bound so the test doesn't actually crash if the bug is
  // present — we stop after `kCap` frames and assert we reached it
  // inside a single outer Fetch call.
  static constexpr int kCap = 2000;

  void OnFetched(const NodeRef& node) {
    ++depth;
    if (depth > max_depth)
      max_depth = depth;

    if (depth >= kCap) {
      --depth;
      return;
    }

    // The bug: we call Fetch on the child from inside the parent's
    // callback. With synchronous fetch completion, the child's
    // callback fires inline — unbounded recursion.
    NodeRef next;
    if (const auto& m = node.model()) {
      const auto* chain = dynamic_cast<const ChainNodeModel*>(m.get());
      assert(chain);
      next = chain->child();
    }

    if (next) {
      next.Fetch(NodeFetchStatus::NodeOnly(),
                 [this](const NodeRef& child) { OnFetched(child); });
    }

    --depth;
  }
};

}  // namespace

// The sync-callback path in `BaseNodeModel::Fetch` invokes the
// callback in the caller's stack frame when the requested status is
// already satisfied. If the callback calls `Fetch` on a child that is
// also already-fetched, the child's callback fires inline too — one
// frame per tree level. The screenshot generator hits this during
// boot with 74+ pending nodes; the chain overflows long before
// production gRPC-backed fetches ever would.
//
// This test chains 2000 models head-to-tail and fetches the head
// with a callback that walks into each child. If the walk reaches
// `kCap` frames in a single synchronous call, the recursion is
// unbounded in principle — the cap exists only to keep the test
// process alive.
TEST(NodeRefFetchRecursion, CallbackThatFetchesChildRecursesSynchronously) {
  // Build a head → tail chain. Each link's `child` is the next link;
  // the last link's child is null so the recursion terminates once
  // the walker runs out of chain.
  constexpr int kChainLength = RecursiveWalker::kCap;
  std::vector<std::shared_ptr<ChainNodeModel>> chain;
  chain.reserve(kChainLength);
  for (int i = 0; i < kChainLength; ++i)
    chain.push_back(ChainNodeModel::Create());
  for (int i = 0; i + 1 < kChainLength; ++i)
    chain[i]->set_child(NodeRef{chain[i + 1]});

  RecursiveWalker walker;
  NodeRef head{chain.front()};
  head.Fetch(NodeFetchStatus::NodeOnly(),
             [&walker](const NodeRef& node) { walker.OnFetched(node); });

  // If `BaseNodeModel::Fetch` deferred the callback instead of running
  // it inline, `max_depth` would be 1 (only the initial frame). Any
  // value greater than 1 demonstrates synchronous re-entry; reaching
  // the cap demonstrates unbounded recursion.
  EXPECT_GT(walker.max_depth, 1)
      << "Fetch callback did not re-enter synchronously — the sync "
         "path in BaseNodeModel::Fetch has changed and this test is "
         "no longer exercising the recursion it was written to expose.";
  EXPECT_EQ(walker.max_depth, RecursiveWalker::kCap)
      << "Expected the recursion to run unbounded up to the cap. If "
         "this assertion fails the fetch chain was broken up — "
         "likely by posting the callback through an executor.";
}
