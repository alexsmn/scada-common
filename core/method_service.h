#pragma once

#include <functional>
#include <vector>

namespace scada {

class NodeId;
class Status;
class Variant;

typedef std::function<void(const Status&)> StatusCallback;

class MethodService {
 public:
  virtual void Call(const NodeId& node_id, const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const StatusCallback& callback) = 0;
};

} // namespace scada