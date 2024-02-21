#include "node_service/node_promises.h"

#include "node_service/node_service.h"

#include <queue>
#include <ranges>

scada::status_promise<void> FetchNode(const NodeRef& node) {
  if (node.fetched()) {
    return scada::MakeResolvedStatusPromise();
  }

  scada::status_promise<void> promise;
  node.Fetch(NodeFetchStatus::NodeOnly(),
             [promise](const NodeRef& node) mutable {
               scada::ResolveStatusPromise(promise, std::move(node.status()));
             });
  return promise;
}

scada::status_promise<void> FetchChildren(const NodeRef& node) {
  if (node.children_fetched()) {
    return scada::MakeResolvedStatusPromise();
  }

  scada::status_promise<void> promise;
  node.Fetch(NodeFetchStatus::NodeAndChildren(),
             [promise](const NodeRef& node) mutable {
               assert(node.fetched());
               assert(node.type_definition().fetched());
               assert(node.children_fetched());
               scada::ResolveStatusPromise(promise, std::move(node.status()));
             });
  return promise;
}

scada::status_promise<void> FetchRecursive(const NodeRef& node,
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

scada::status_promise<void> FetchTypeSystem(NodeService& node_service) {
  return FetchRecursive(node_service.GetNode(scada::id::TypesFolder),
                        scada::id::HierarchicalReferences);
}