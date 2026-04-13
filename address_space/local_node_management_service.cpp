#include "address_space/local_node_management_service.h"

#include "scada/status.h"

namespace scada {

void LocalNodeManagementService::AddNodes(
    const std::vector<AddNodesItem>& inputs,
    const AddNodesCallback& callback) {
  std::vector<AddNodesResult> results(
      inputs.size(), AddNodesResult{.status_code = StatusCode::Bad});
  callback(Status{StatusCode::Bad}, std::move(results));
}

void LocalNodeManagementService::DeleteNodes(
    const std::vector<DeleteNodesItem>& inputs,
    const DeleteNodesCallback& callback) {
  std::vector<StatusCode> results(inputs.size(), StatusCode::Bad);
  callback(Status{StatusCode::Bad}, std::move(results));
}

void LocalNodeManagementService::AddReferences(
    const std::vector<AddReferencesItem>& inputs,
    const AddReferencesCallback& callback) {
  std::vector<StatusCode> results(inputs.size(), StatusCode::Bad);
  callback(Status{StatusCode::Bad}, std::move(results));
}

void LocalNodeManagementService::DeleteReferences(
    const std::vector<DeleteReferencesItem>& inputs,
    const DeleteReferencesCallback& callback) {
  std::vector<StatusCode> results(inputs.size(), StatusCode::Bad);
  callback(Status{StatusCode::Bad}, std::move(results));
}

}  // namespace scada
