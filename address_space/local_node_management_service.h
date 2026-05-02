#pragma once

#include "scada/node_management_service.h"

namespace scada {

// NodeManagementService stub for test/fixture back-ends that never mutate
// the node graph. Every mutation completes synchronously with a generic
// `Bad` status. Exists so fixtures can satisfy
// `scada::services::node_management_service` (dereferenced by
// `DataServices::FromSharedServices`) without pulling in a real back-end.
class LocalNodeManagementService : public NodeManagementService {
 public:
  Awaitable<StatusOr<std::vector<AddNodesResult>>> AddNodes(
      std::vector<AddNodesItem> inputs) override;
  Awaitable<StatusOr<std::vector<StatusCode>>> DeleteNodes(
      std::vector<DeleteNodesItem> inputs) override;
  Awaitable<StatusOr<std::vector<StatusCode>>> AddReferences(
      std::vector<AddReferencesItem> inputs) override;
  Awaitable<StatusOr<std::vector<StatusCode>>> DeleteReferences(
      std::vector<DeleteReferencesItem> inputs) override;
};

}  // namespace scada
