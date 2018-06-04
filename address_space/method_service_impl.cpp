#include "method_service_impl.h"

#include <cassert>

#include "address_space/address_space_util.h"
#include "address_space/node.h"

MethodServiceImpl::MethodServiceImpl(const MethodServiceImplContext& context)
    : MethodServiceImplContext{context} {}

void MethodServiceImpl::Call(const scada::NodeId& node_id,
                             const scada::NodeId& method_id,
                             const std::vector<scada::Variant>& arguments,
                             const scada::NodeId& user_id,
                             const scada::StatusCallback& callback) {
  base::StringPiece nested_name;
  auto* node = GetNestedNode(address_space_, node_id, nested_name);
  if (!node)
    return callback(scada::StatusCode::Bad_WrongNodeId);

  auto* node_service = node->GetMethodService();
  if (!node_service)
    return callback(scada::StatusCode::Bad_WrongMethodId);

  node_service->Call(node_id, method_id, arguments, user_id, callback);
}
