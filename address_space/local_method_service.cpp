#include "address_space/local_method_service.h"

#include "scada/status.h"

namespace scada {

void LocalMethodService::Call(const NodeId& /*node_id*/,
                              const NodeId& /*method_id*/,
                              const std::vector<Variant>& /*arguments*/,
                              const NodeId& /*user_id*/,
                              const StatusCallback& callback) {
  callback(Status{StatusCode::Bad});
}

Awaitable<Status> LocalMethodService::Call(NodeId /*node_id*/,
                                           NodeId /*method_id*/,
                                           std::vector<Variant> /*arguments*/,
                                           NodeId /*user_id*/) {
  co_return Status{StatusCode::Bad};
}

}  // namespace scada
