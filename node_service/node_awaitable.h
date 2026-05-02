#pragma once

#include "base/awaitable.h"

namespace scada {
class NodeId;
}

class NodeRef;
class NodeService;

Awaitable<void> FetchNode(const NodeRef& node);
Awaitable<void> FetchChildren(const NodeRef& node);
Awaitable<void> WaitForPendingNodes(NodeService& node_service);

// TODO: Combine with `FetchTree`.
Awaitable<void> FetchRecursive(const NodeRef& node,
                               const scada::NodeId& ref_type_id);

Awaitable<void> FetchTypeSystem(NodeService& node_service);
