#pragma once

#include "base/awaitable.h"
#include "common/node_state.h"
#include "scada/co_result.h"
#include "scada/status_or.h"

namespace v3 {

struct NodeFetcherContext {};

// Coroutine fetch facade used by v3::NodeServiceImpl.
//
// v3 depends on this small abstraction rather than on a concrete fetcher,
// keeping remote loading policy outside the service. ServiceNodeFetcher is the
// production implementation.
class NodeFetcher : private NodeFetcherContext {
 public:
  virtual ~NodeFetcher() = default;

  virtual scada::CoStatusOr<scada::NodeState> FetchNode(
      const scada::NodeId& node_id);

  virtual scada::CoStatusOr<scada::ReferenceDescriptions> FetchChildren(
      const scada::NodeId& node_id);
};

}  // namespace v3
