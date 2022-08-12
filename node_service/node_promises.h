#pragma once

#include "base/promise.h"
#include "node_service/node_service.h"

inline promise<void> FetchNode(const NodeRef& node) {
  if (node.fetched())
    return make_resolved_promise();

  auto promise = make_promise<void>();
  node.Fetch(NodeFetchStatus::NodeOnly(),
             [promise](const NodeRef& node) mutable {
               assert(node.fetched());
               assert(node.type_definition().fetched());
               promise.resolve();
             });
  return promise;
}

inline promise<void> FetchChildren(const NodeRef& node) {
  if (node.children_fetched())
    return make_resolved_promise();

  promise<void> promise;
  node.Fetch(NodeFetchStatus::NodeAndChildren(),
             [promise](const NodeRef& node) mutable {
               assert(node.fetched());
               assert(node.type_definition().fetched());
               assert(node.children_fetched());
               promise.resolve();
             });
  return promise;
}
