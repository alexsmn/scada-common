// Regression coverage for already-satisfied NodeRef coroutine fetches.

#include "node_service/base_node_model.h"
#include "node_service/node_ref.h"

#include "base/awaitable.h"

#include <boost/asio/io_context.hpp>

#include <gtest/gtest.h>

#include <cassert>
#include <memory>

namespace {

// Chain link: a BaseNodeModel that reports `NodeOnly` as already fetched and
// exposes the next link as its child.
class ChainNodeModel : public BaseNodeModel {
 public:
  static std::shared_ptr<ChainNodeModel> Create() {
    auto model = std::shared_ptr<ChainNodeModel>{new ChainNodeModel{}};
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

TEST(NodeRefFetchRecursion, SameModelCoroutineFetchCanRepeat) {
  auto model = ChainNodeModel::Create();
  NodeRef node{model};

  constexpr int kRepeat = 2000;

  int fetch_count = 0;
  boost::asio::io_context io_context;
  RunAwaitable(io_context, [&]() -> Awaitable<void> {
    for (int i = 0; i < kRepeat; ++i) {
      co_await node.Fetch(NodeFetchStatus::NodeOnly());
      ++fetch_count;
    }
  });

  EXPECT_EQ(fetch_count, kRepeat);
}

TEST(NodeRefFetchRecursion, CrossModelCoroutineFetchCanWalkChain) {
  constexpr int kChainLength = 64;
  std::vector<std::shared_ptr<ChainNodeModel>> chain;
  chain.reserve(kChainLength);
  for (int i = 0; i < kChainLength; ++i)
    chain.push_back(ChainNodeModel::Create());
  for (int i = 0; i + 1 < kChainLength; ++i)
    chain[i]->set_child(NodeRef{chain[i + 1]});

  int fetch_count = 0;
  boost::asio::io_context io_context;
  RunAwaitable(io_context, [&]() -> Awaitable<void> {
    for (NodeRef node{chain.front()}; node;) {
      co_await node.Fetch(NodeFetchStatus::NodeOnly());
      ++fetch_count;
      NodeRef next;
        if (const auto& m = node.model()) {
          const auto* link = dynamic_cast<const ChainNodeModel*>(m.get());
          assert(link);
          next = link->child();
        }
      node = std::move(next);
    }
  });

  EXPECT_EQ(fetch_count, kChainLength);
}
