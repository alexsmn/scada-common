#include "address_space/local_method_service.h"

#include "scada/status.h"

namespace scada {

Awaitable<Status> LocalMethodService::Call(
    NodeId /*node_id*/,
    NodeId /*method_id*/,
    std::vector<Variant> /*arguments*/,
    ServiceContext /*context*/) {
  co_return Status{StatusCode::Bad};
}

}  // namespace scada
