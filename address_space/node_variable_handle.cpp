#include "address_space/node_variable_handle.h"

#include "address_space/variable.h"

namespace scada {

NodeVariableHandle::NodeVariableHandle(Variable& node) : node_(node) {}

scada::Node& NodeVariableHandle::GetNode() {
  return node_;
}

void NodeVariableHandle::Write(
    const scada::ServiceContext& context,
    const scada::WriteValue& write_value,
    const scada::StatusCallback& callback) {
  node_.Write(context, write_value, callback);
}

void NodeVariableHandle::Call(const scada::NodeId& method_id,
                              const std::vector<scada::Variant>& arguments,
                              const scada::NodeId& user_id,
                              const scada::StatusCallback& callback) {
  node_.Call(method_id, arguments, user_id, callback);
}

}  // namespace scada
