#pragma once

#include "base/promise.h"

namespace scada {
class NodeId;
}

class NodeRef;
class NodeService;

promise<void> FetchNode(const NodeRef& node);
promise<void> FetchChildren(const NodeRef& node);

// TODO: Combine with `FetchTree`.
promise<void> FetchRecursive(const NodeRef& node,
                                           const scada::NodeId& ref_type_id);

promise<void> FetchTypeSystem(NodeService& node_service);
