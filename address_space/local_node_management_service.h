#pragma once

#include "scada/co_result.h"
#include "scada/node_management_service.h"

namespace scada {

// NodeManagementService stub for test/fixture back-ends that never mutate
// the node graph. Every mutation completes synchronously with a generic
// `Bad` status. Exists so fixtures can satisfy
// `scada::services::node_management_service` (dereferenced by
// `DataServices::FromSharedServices`) without pulling in a real back-end.
class LocalNodeManagementService : public NodeManagementService {
 public:
  CoStatusOr<std::vector<AddNodesResult>> AddNodes(
      ServiceContext context,
      std::vector<AddNodesItem> inputs) override;
  CoStatusOr<std::vector<StatusCode>> DeleteNodes(
      ServiceContext context,
      std::vector<DeleteNodesItem> inputs) override;
  CoStatusOr<std::vector<StatusCode>> AddReferences(
      ServiceContext context,
      std::vector<AddReferencesItem> inputs) override;
  CoStatusOr<std::vector<StatusCode>> DeleteReferences(
      ServiceContext context,
      std::vector<DeleteReferencesItem> inputs) override;
};

}  // namespace scada
