#pragma once

#include "base/awaitable.h"
#include "node_service/node_ref.h"

#include <queue>

class NodeService;

bool IsSubtypeOf(NodeRef type_definition, const scada::NodeId& supertype_id);

bool IsInstanceOf(const NodeRef& node, const scada::NodeId& type_definition_id);

std::vector<NodeRef> GetDataVariables(const NodeRef& node);

std::u16string GetFullDisplayName(const NodeRef& node);

scada::LocalizedText GetDisplayName(NodeService& node_service,
                                    const scada::NodeId& node_id);

// TODO: Combine with `FetchRecursive`.
inline Awaitable<void> FetchTree(const NodeRef& root) {
  std::queue<NodeRef> queue;
  queue.emplace(root);

  while (!queue.empty()) {
    auto node = std::move(queue.front());
    queue.pop();

    node = co_await node.Fetch(NodeFetchStatus::NodeAndChildren);
    for (const auto& child : node.targets(scada::id::Organizes)) {
      queue.emplace(child);
    }
  }
}
