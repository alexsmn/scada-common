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
  void AddNodes(const std::vector<AddNodesItem>& inputs,
                const AddNodesCallback& callback) override;
  void DeleteNodes(const std::vector<DeleteNodesItem>& inputs,
                   const DeleteNodesCallback& callback) override;
  void AddReferences(const std::vector<AddReferencesItem>& inputs,
                     const AddReferencesCallback& callback) override;
  void DeleteReferences(const std::vector<DeleteReferencesItem>& inputs,
                        const DeleteReferencesCallback& callback) override;
};

}  // namespace scada
