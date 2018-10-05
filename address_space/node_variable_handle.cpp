#include "address_space/node_variable_handle.h"

#include "address_space/variable.h"

namespace scada {

NodeVariableHandle::NodeVariableHandle(Variable& node) : node_(node) {}

scada::Node& NodeVariableHandle::GetNode() {
  return node_;
}

void NodeVariableHandle::Write(scada::AttributeId attribute_id,
                               const scada::Variant& value,
                               const WriteFlags& flags,
                               const NodeId& user_id,
                               const StatusCallback& callback) {
  node_.Write(attribute_id, value, flags, user_id, callback);
}

void NodeVariableHandle::Call(const scada::NodeId& method_id,
                              const std::vector<scada::Variant>& arguments,
                              const scada::NodeId& user_id,
                              const scada::StatusCallback& callback) {
  node_.Call(method_id, arguments, user_id, callback);
}

}  // namespace scada
