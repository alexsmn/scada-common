#include "address_space/local_node_management_service.h"

#include "scada/co_result.h"
#include "scada/status.h"

namespace scada {

CoStatusOr<std::vector<AddNodesResult>> LocalNodeManagementService::AddNodes(
    ServiceContext /*context*/,
    std::vector<AddNodesItem> inputs) {
  co_return Status{StatusCode::Bad};
}

CoStatusOr<std::vector<StatusCode>> LocalNodeManagementService::DeleteNodes(
    ServiceContext /*context*/,
    std::vector<DeleteNodesItem> inputs) {
  co_return Status{StatusCode::Bad};
}

CoStatusOr<std::vector<StatusCode>> LocalNodeManagementService::AddReferences(
    ServiceContext /*context*/,
    std::vector<AddReferencesItem> inputs) {
  co_return Status{StatusCode::Bad};
}

CoStatusOr<std::vector<StatusCode>>
LocalNodeManagementService::DeleteReferences(
    ServiceContext /*context*/,
    std::vector<DeleteReferencesItem> inputs) {
  co_return Status{StatusCode::Bad};
}

}  // namespace scada
