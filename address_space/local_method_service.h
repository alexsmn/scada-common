#pragma once

#include "scada/method_service.h"

namespace scada {

// MethodService stub for test/fixture back-ends that never execute a method
// call. Every call completes synchronously with a generic `Bad` status.
// Exists so fixtures can satisfy `v1::NodeServiceContext::method_service_`
// (which is a non-null reference) without pulling in a full address-space-
// backed implementation.
class LocalMethodService : public MethodService {
 public:
  void Call(const NodeId& node_id,
            const NodeId& method_id,
            const std::vector<Variant>& arguments,
            const NodeId& user_id,
            const StatusCallback& callback) override;
};

}  // namespace scada
