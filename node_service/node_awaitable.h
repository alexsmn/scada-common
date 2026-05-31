#pragma once

#include "base/awaitable.h"
#include "scada/status.h"

namespace scada {
class NodeId;
}

class NodeRef;
class NodeService;

Awaitable<void> FetchNode(const NodeRef& node);
Awaitable<void> FetchChildren(const NodeRef& node);
Awaitable<void> WaitForPendingNodes(NodeService& node_service);
Awaitable<scada::Status> FetchNodeStatus(const NodeRef& node);
Awaitable<scada::Status> FetchChildrenStatus(const NodeRef& node);

// TODO: Combine with `FetchTree`.
Awaitable<void> FetchRecursive(const NodeRef& node,
                               const scada::NodeId& ref_type_id);
Awaitable<scada::Status> FetchRecursiveStatus(
    const NodeRef& node,
    const scada::NodeId& ref_type_id);

Awaitable<void> FetchTypeSystem(NodeService& node_service);
Awaitable<scada::Status> FetchTypeSystemStatus(NodeService& node_service);
