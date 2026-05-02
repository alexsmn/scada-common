#include "address_space/local_node_management_service.h"

#include "scada/status.h"

namespace scada {

Awaitable<StatusOr<std::vector<AddNodesResult>>>
LocalNodeManagementService::AddNodes(std::vector<AddNodesItem> inputs) {
  co_return Status{StatusCode::Bad};
}

Awaitable<StatusOr<std::vector<StatusCode>>>
LocalNodeManagementService::DeleteNodes(std::vector<DeleteNodesItem> inputs) {
  co_return Status{StatusCode::Bad};
}

Awaitable<StatusOr<std::vector<StatusCode>>>
LocalNodeManagementService::AddReferences(
    std::vector<AddReferencesItem> inputs) {
  co_return Status{StatusCode::Bad};
}

Awaitable<StatusOr<std::vector<StatusCode>>>
LocalNodeManagementService::DeleteReferences(
    std::vector<DeleteReferencesItem> inputs) {
  co_return Status{StatusCode::Bad};
}

}  // namespace scada
