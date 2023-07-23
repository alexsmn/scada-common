#pragma once

#include "scada/status_promise.h"
#include "node_service/node_service.h"

inline promise<> FetchNode(const NodeRef& node) {
  if (node.fetched())
    return make_resolved_promise();

  promise<> promise;
  node.Fetch(NodeFetchStatus::NodeOnly(),
             [promise](const NodeRef& node) mutable {
               scada::ResolveStatusPromise(promise, std::move(node.status()));
             });
  return promise;
}

inline promise<> FetchChildren(const NodeRef& node) {
  if (node.children_fetched())
    return make_resolved_promise();

  promise<> promise;
  node.Fetch(NodeFetchStatus::NodeAndChildren(),
             [promise](const NodeRef& node) mutable {
               assert(node.fetched());
               assert(node.type_definition().fetched());
               assert(node.children_fetched());
               scada::ResolveStatusPromise(promise, std::move(node.status()));
             });
  return promise;
}
