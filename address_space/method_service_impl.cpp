#include "method_service_impl.h"
#include "scada/co_result.h"

MethodServiceImpl::MethodServiceImpl(const MethodServiceImplContext& context)
    : MethodServiceImplContext{context} {}

scada::CoStatusOr<scada::CallResult> MethodServiceImpl::Call(
    scada::NodeId /*node_id*/,
    scada::NodeId /*method_id*/,
    std::vector<scada::Variant> /*arguments*/,
    scada::ServiceContext /*context*/) {
  co_return scada::Status{scada::StatusCode::Bad_WrongMethodId};
}
