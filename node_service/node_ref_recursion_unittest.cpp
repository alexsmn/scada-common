// Regression test for the synchronous Fetch-callback recursion in
// `BaseNodeModel::Fetch`. Historically the already-satisfied path
// invoked the callback inline in the caller's stack frame; a callback
// that called Fetch() on the same model would re-enter unbounded.
//
// The fix routes the "already satisfied" branch through the shared
// callback queue and drains it iteratively in `NotifyCallbacks()` —
// nested calls return immediately and the outer while-loop picks up
// whatever the callback enqueued. Scope: the drain guard is
// per-model (`notifying_callbacks_`), so callbacks that cross into a
// *different* NodeModel via `NodeRef::Fetch` on a child can still
// re-enter. A cross-model guard would need a thread-local registry
// of pending models and is left for a follow-up — the fetcher-queue
// drain (`NodeFetcherImpl::draining_pending_queue_`) is the real
// production path and is fixed separately.

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

}  // namespace

// Same-model recursion: a callback that calls `Fetch()` again on the
// same NodeRef must not re-enter on the caller's stack. The fix's
// iterative `NotifyCallbacks()` drain pops ready callbacks, runs
// them, and loops — nested `Fetch()` calls enqueue and the outer
// loop picks them up. `max_depth == 1` proves the drain is flat.
TEST(NodeRefFetchRecursion, SameModelCallbackReentryIsNotRecursive) {
  auto model = ChainNodeModel::Create();
  NodeRef node{model};

  constexpr int kRepeat = 2000;

  int depth = 0;
  int max_depth = 0;
  int callback_invocations = 0;

  // Recursive closure: every invocation schedules another Fetch on
  // the same node until `kRepeat` total invocations have run.
  std::function<void(const NodeRef&)> on_fetched =
      [&](const NodeRef& self) {
        ++depth;
        if (depth > max_depth)
          max_depth = depth;
        ++callback_invocations;

        if (callback_invocations < kRepeat)
          self.Fetch(NodeFetchStatus::NodeOnly(), on_fetched);

        --depth;
      };

  node.Fetch(NodeFetchStatus::NodeOnly(), on_fetched);

  EXPECT_EQ(callback_invocations, kRepeat);
  EXPECT_EQ(max_depth, 1)
      << "Same-model Fetch callbacks re-entered on the caller's stack — "
         "the iterative NotifyCallbacks() drain is no longer taking "
         "effect. max_depth=" << max_depth;
}

// Cross-model recursion: a callback that calls `Fetch()` on a
// *different* NodeModel is currently NOT bounded by the per-model
// drain guard — each model's `notifying_callbacks_` is independent.
// This test locks in the current (known-limited) behavior so the
// next person to touch this path sees the gap and the reasoning.
// Flip the EXPECT_GT to EXPECT_EQ(1) if/when cross-model draining
// lands (e.g. via a thread-local drain registry).
TEST(NodeRefFetchRecursion, CrossModelChainIsKnownLimitation) {
  constexpr int kChainLength = 2000;
  std::vector<std::shared_ptr<ChainNodeModel>> chain;
  chain.reserve(kChainLength);
  for (int i = 0; i < kChainLength; ++i)
    chain.push_back(ChainNodeModel::Create());
  for (int i = 0; i + 1 < kChainLength; ++i)
    chain[i]->set_child(NodeRef{chain[i + 1]});

  int depth = 0;
  int max_depth = 0;

  std::function<void(const NodeRef&)> on_fetched =
      [&](const NodeRef& node) {
        ++depth;
        if (depth > max_depth)
          max_depth = depth;

        NodeRef next;
        if (const auto& m = node.model()) {
          const auto* link = dynamic_cast<const ChainNodeModel*>(m.get());
          assert(link);
          next = link->child();
        }
        if (next)
          next.Fetch(NodeFetchStatus::NodeOnly(), on_fetched);

        --depth;
      };

  NodeRef head{chain.front()};
  head.Fetch(NodeFetchStatus::NodeOnly(), on_fetched);

  EXPECT_GT(max_depth, 1)
      << "Cross-model Fetch callbacks stopped recursing — the fix's "
         "scope has grown. Update this test to assert bounded depth "
         "and remove the `known limitation` note.";
}
