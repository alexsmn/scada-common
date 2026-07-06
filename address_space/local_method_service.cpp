#include "address_space/local_method_service.h"

// A coroutine-defining TU needs std::coroutine_traits textually even when
// all names come from the import.
#include <coroutine>

#if defined(SCADA_USE_CORE_MODULE)
// Modules-pilot consumer (SCADA_CXX_MODULES=ON): scada names come from the
// scada.core facade. The import sits after the textual includes because the
// reverse order trips an AppleClang 21 declaration-merging bug in libc++.
import scada.core;
#else
#include "scada/status.h"
#endif

namespace scada {

Awaitable<Status> LocalMethodService::Call(NodeId /*node_id*/,
                                           NodeId /*method_id*/,
                                           std::vector<Variant> /*arguments*/,
                                           ServiceContext /*context*/) {
  co_return Status{StatusCode::Bad};
}

}  // namespace scada
