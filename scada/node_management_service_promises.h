#pragma once

#include "scada/node_management_service.h"
#include "base/promise.h"

namespace scada {

inline promise<std::vector<AddNodesResult>> AddNode(
    NodeManagementService& service,
    const std::vector<AddNodesItem>& inputs) {
  promise<std::vector<AddNodesResult>> p;
  service.AddNodes(
      inputs, [p](Status status, std::vector<AddNodesResult> results) mutable {
        if (status) {
          p.resolve(std::move(results));
        } else {
          RejectStatusPromise(p, std::move(status));
        }
      });
  return p;
}

inline promise<NodeId> AddNode(NodeManagementService& service,
                                      const AddNodesItem& input) {
  promise<NodeId> p;

  service.AddNodes(
      {input}, [p](Status status, std::vector<AddNodesResult> results) mutable {
        if (!status) {
          RejectStatusPromise(p, std::move(status));
          return;
        }

        assert(results.size() == 1);

        auto& [status_code, node_id] = results[0];
        if (IsBad(status_code)) {
          RejectStatusPromise(p, status_code);
          return;
        }

        p.resolve(std::move(node_id));
      });

  return p;
}

inline promise<void> DeleteNode(NodeManagementService& service,
                                       const DeleteNodesItem& input) {
  promise<void> p;

  service.DeleteNodes(
      {input}, [p](Status status, std::vector<StatusCode> results) mutable {
        if (!status) {
          RejectStatusPromise(p, std::move(status));
          return;
        }

        assert(results.size() == 1);

        auto status_code = results[0];
        CompleteStatusPromise(p, status_code);
      });

  return p;
}

}  // namespace scada