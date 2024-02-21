#pragma once

#include "scada/status_promise.h"

namespace scada {
class NodeId;
}

class NodeRef;
class NodeService;

scada::status_promise<void> FetchNode(const NodeRef& node);
scada::status_promise<void> FetchChildren(const NodeRef& node);

// TODO: Combine with `FetchTree`.
scada::status_promise<void> FetchRecursive(const NodeRef& node,
                                           const scada::NodeId& ref_type_id);

scada::status_promise<void> FetchTypeSystem(NodeService& node_service);
