#include "method_service_impl.h"

MethodServiceImpl::MethodServiceImpl(const MethodServiceImplContext& context)
    : MethodServiceImplContext{context} {}

void MethodServiceImpl::Call(const scada::NodeId& node_id,
                             const scada::NodeId& method_id,
                             const std::vector<scada::Variant>& arguments,
                             const scada::NodeId& user_id,
                             const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad_WrongMethodId);
}
