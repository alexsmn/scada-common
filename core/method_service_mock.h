#pragma once

#include "core/method_service.h"

#include <gmock/gmock.h>

namespace scada {

class MockMethodService : public MethodService {
 public:
  MOCK_METHOD5(Call,
               void(const NodeId& node_id,
                    const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback));
};

}  // namespace scada
