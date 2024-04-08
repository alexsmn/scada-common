#include "node_service/node_promises.h"

#include "node_service/node_service.h"
#include "scada/status_promise.h"

#include <queue>
#include <ranges>

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