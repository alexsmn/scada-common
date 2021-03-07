#pragma once

#include "base/promise.h"
#include "common/node_state.h"

namespace v3 {

struct NodeFetcherContext {};

class NodeFetcher : private NodeFetcherContext {
 public:
  promise<scada::NodeState> FetchNode(const scada::NodeId& node_id);

  promise<std::vector<scada::NodeId>> FetchChildren(
      const scada::NodeId& node_id);
};

}  // namespace v3
