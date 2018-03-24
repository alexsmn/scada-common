#pragma once

#include "core/node_id.h"
#include "core/status.h"
#include "core/variant.h"

#include <functional>
#include <vector>

namespace scada {

using StatusCallback = std::function<void(const Status&)>;

class MethodService {
 public:
  virtual ~MethodService() {}

  virtual void Call(const NodeId& node_id,
                    const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback) = 0;
};

}  // namespace scada
