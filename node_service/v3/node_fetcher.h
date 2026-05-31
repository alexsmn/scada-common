#pragma once

#include "base/awaitable.h"
#include "common/node_state.h"
#include "scada/status_or.h"

namespace v3 {

struct NodeFetcherContext {};

// Coroutine fetch facade used by v3::NodeServiceImpl.
//
// v3 depends on this small abstraction instead of constructing the concrete
// NodeFetcherImpl/NodeChildrenFetcher pair used by v2, keeping remote loading
// policy outside the service.
class NodeFetcher : private NodeFetcherContext {
 public:
  virtual ~NodeFetcher() = default;

  virtual Awaitable<scada::StatusOr<scada::NodeState>> FetchNode(
      const scada::NodeId& node_id);

  virtual Awaitable<scada::StatusOr<scada::ReferenceDescriptions>> FetchChildren(
      const scada::NodeId& node_id);
};

}  // namespace v3
