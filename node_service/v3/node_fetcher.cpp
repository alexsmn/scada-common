#include "node_service/v3/node_fetcher.h"

namespace v3 {

Awaitable<scada::NodeState> NodeFetcher::FetchNode(
    const scada::NodeId& node_id) {
  co_return scada::NodeState{};
}

Awaitable<std::vector<scada::NodeId>> NodeFetcher::FetchChildren(
    const scada::NodeId& node_id) {
  co_return std::vector<scada::NodeId>();
}

}  // namespace v3
