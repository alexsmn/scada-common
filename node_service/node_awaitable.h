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

// Fetches a type definition and its whole supertype chain, children included.
// Node fetches are per node: a fetched instance only records its
// type-definition and supertype ids, and a type's HasProperty / HasSubtype
// targets only become visible once that type's children are fetched — so any
// synchronous walk over a type hierarchy (property declarations, subtype
// checks, aggregate lookups) must be preceded by this fetch.
Awaitable<scada::Status> FetchTypeChainStatus(NodeRef type_definition);

// TODO: Combine with `FetchTree`.
Awaitable<void> FetchRecursive(const NodeRef& node,
                               const scada::NodeId& ref_type_id);
Awaitable<scada::Status> FetchRecursiveStatus(
    const NodeRef& node,
    const scada::NodeId& ref_type_id);

Awaitable<void> FetchTypeSystem(NodeService& node_service);
Awaitable<scada::Status> FetchTypeSystemStatus(NodeService& node_service);
