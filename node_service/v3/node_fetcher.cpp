#include "node_service/v3/node_fetcher.h"
#include "scada/co_result.h"

namespace v3 {

scada::CoStatusOr<scada::NodeState> NodeFetcher::FetchNode(
    const scada::NodeId& node_id) {
  co_return scada::NodeState{};
}

scada::CoStatusOr<scada::ReferenceDescriptions> NodeFetcher::FetchChildren(
    const scada::NodeId& node_id) {
  co_return scada::ReferenceDescriptions{};
}

}  // namespace v3
