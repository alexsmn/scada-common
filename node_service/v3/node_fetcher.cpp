#include "node_service/v3/node_fetcher.h"

namespace v3 {

Awaitable<scada::StatusOr<scada::NodeState>> NodeFetcher::FetchNode(
    const scada::NodeId& node_id) {
  co_return scada::NodeState{};
}

Awaitable<scada::StatusOr<scada::ReferenceDescriptions>> NodeFetcher::FetchChildren(
    const scada::NodeId& node_id) {
  co_return scada::ReferenceDescriptions{};
}

}  // namespace v3
