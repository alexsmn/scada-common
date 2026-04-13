#include "node_service/node_promises.h"

#include "node_service/node_observer.h"
#include "node_service/node_service.h"
#include "scada/status_promise.h"

#include <queue>
#include <ranges>
#include <memory>

namespace {

class PendingNodesWaiter final : private NodeRefObserver,
                                 public std::enable_shared_from_this<
                                     PendingNodesWaiter> {
 public:
  explicit PendingNodesWaiter(NodeService& node_service)
      : node_service_{node_service} {}

  promise<void> Wait() {
    if (node_service_.GetPendingTaskCount() == 0) {
      return make_resolved_promise();
    }

    node_service_.Subscribe(*this);
    subscribed_ = true;
    TryResolve();

    return promise_.then([keep_alive = shared_from_this()] {});
  }

  ~PendingNodesWaiter() {
    if (subscribed_) {
      node_service_.Unsubscribe(*this);
    }
  }

 private:
  void OnNodeFetched(const NodeFetchedEvent&) override { TryResolve(); }

  void TryResolve() {
    if (resolved_ || node_service_.GetPendingTaskCount() != 0) {
      return;
    }

    resolved_ = true;
    if (subscribed_) {
      node_service_.Unsubscribe(*this);
      subscribed_ = false;
    }
    promise_.resolve();
  }

  NodeService& node_service_;
  promise<void> promise_;
  bool subscribed_ = false;
  bool resolved_ = false;
};

}  // namespace

promise<void> FetchNode(const NodeRef& node) {
  // Optimization to avoid promise allocation.
  if (node.fetched()) {
    return scada::MakeCompletedStatusPromise(node.status());
  }

  promise<void> promise;
  node.Fetch(NodeFetchStatus::NodeOnly(),
             [promise](const NodeRef& node) mutable {
               scada::CompleteStatusPromise(promise, std::move(node.status()));
             });
  return promise;
}

promise<void> FetchChildren(const NodeRef& node) {
  if (node.children_fetched()) {
    return make_resolved_promise();
  }

  promise<void> promise;
  node.Fetch(NodeFetchStatus::NodeAndChildren(),
             [promise](const NodeRef& node) mutable {
               assert(node.fetched());
               assert(node.type_definition().fetched());
               assert(node.children_fetched());
               scada::CompleteStatusPromise(promise, std::move(node.status()));
             });
  return promise;
}

promise<void> WaitForPendingNodes(NodeService& node_service) {
  auto waiter = std::make_shared<PendingNodesWaiter>(node_service);
  return waiter->Wait();
}

promise<void> FetchRecursive(const NodeRef& node,
                             const scada::NodeId& ref_type_id) {
  return FetchChildren(node).then([node, ref_type_id] {
    const auto& targets = node.targets(ref_type_id);
    const auto& target_promises =
        targets | std::views::transform([&](const NodeRef& target) {
          return FetchRecursive(target, ref_type_id);
        });
    return make_all_promise_void(target_promises);
  });
}

promise<void> FetchTypeSystem(NodeService& node_service) {
  return FetchRecursive(node_service.GetNode(scada::id::TypesFolder),
                        scada::id::HierarchicalReferences);
}
