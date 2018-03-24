#include "core/node_variable_handle.h"

#include "core/variable.h"

namespace scada {

NodeVariableHandle::NodeVariableHandle(Variable& node) : node_(node) {}

scada::Node& NodeVariableHandle::GetNode() {
  return node_;
}

void NodeVariableHandle::Write(double value,
                               const scada::NodeId& user_id,
                               const scada::WriteFlags& flags,
                               const scada::StatusCallback& callback) {
  node_.Write(value, user_id, flags, callback);
}

void NodeVariableHandle::Call(const scada::NodeId& method_id,
                              const std::vector<scada::Variant>& arguments,
                              const scada::NodeId& user_id,
                              const scada::StatusCallback& callback) {
  node_.Call(method_id, arguments, user_id, callback);
}

}  // namespace scada
