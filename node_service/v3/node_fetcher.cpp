#include "node_service/v3/node_fetcher.h"

namespace v3 {

promise<scada::NodeState> NodeFetcher::FetchNode(const scada::NodeId& node_id) {
  return make_resolved_promise(scada::NodeState{});
}

promise<std::vector<scada::NodeId>> NodeFetcher::FetchChildren(
    const scada::NodeId& node_id) {
  return make_resolved_promise(std::vector<scada::NodeId>());
}

}  // namespace v3
