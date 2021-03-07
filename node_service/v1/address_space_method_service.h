#pragma once

#include "address_space/address_space_util.h"
#include "address_space/node.h"
#include "core/method_service.h"

namespace scada {
class AddressSpace;
}  // namespace scada

class AddressSpaceMethodService : public scada::MethodService {
 public:
  explicit AddressSpaceMethodService(scada::AddressSpace& address_space)
      : address_space_{address_space} {}

  // scada::MethodService
  virtual void Call(const scada::NodeId& node_id,
                    const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) override;

 private:
  scada::AddressSpace& address_space_;
};

inline void AddressSpaceMethodService::Call(
    const scada::NodeId& node_id,
    const scada::NodeId& method_id,
    const std::vector<scada::Variant>& arguments,
    const scada::NodeId& user_id,
    const scada::StatusCallback& callback) {
  std::string_view nested_name;
  auto* node = GetNestedNode(address_space_, node_id, nested_name);
  if (!node)
    return callback(scada::StatusCode::Bad_WrongNodeId);

  auto* service = node->GetMethodService();
  if (!service)
    return callback(scada::StatusCode::Bad_WrongMethodId);

  service->Call(node_id, method_id, arguments, user_id, callback);
}
