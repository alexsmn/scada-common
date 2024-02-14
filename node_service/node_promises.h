#pragma once

#include "node_service/node_service.h"
#include "scada/status_promise.h"

inline scada::status_promise<void> FetchNode(const NodeRef& node) {
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

inline scada::status_promise<void> FetchChildren(const NodeRef& node) {
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
