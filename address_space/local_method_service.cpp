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

}  // namespace scada
