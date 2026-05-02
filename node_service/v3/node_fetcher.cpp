#include "node_service/v3/node_fetcher.h"

namespace v3 {

Awaitable<scada::NodeState> NodeFetcher::FetchNode(
    const scada::NodeId& node_id) {
  co_return scada::NodeState{};
}

Awaitable<scada::ReferenceDescriptions> NodeFetcher::FetchChildren(
    const scada::NodeId& node_id) {
  co_return scada::ReferenceDescriptions{};
}

}  // namespace v3
