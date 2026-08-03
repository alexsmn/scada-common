#pragma once

#include "base/awaitable.h"
#include "node_service/node_ref.h"

#include <queue>

class NodeService;

bool IsSubtypeOf(NodeRef type_definition, const scada::NodeId& supertype_id);

bool IsInstanceOf(const NodeRef& node, const scada::NodeId& type_definition_id);

// The child instance type definitions that may be created under `node`, read
// from the OPC UA OptionalPlaceholder/MandatoryPlaceholder InstanceDeclarations
// on its type-definition chain (or, when `node` is itself a type, on it and its
// supertypes). This is the standard-modelling replacement for reading the
// proprietary `Creates` reference (OPC UA Part 3 §6.4.4.4.4). Starts the fetches
// it needs; callers re-invoke on model change until the data is resident.
std::vector<NodeRef> GetCreatableChildTypes(const NodeRef& node);

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
