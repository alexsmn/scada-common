#include "opcua/opcua_binary_runtime.h"

namespace opcua {

OpcUaBinaryRuntime::OpcUaBinaryRuntime(OpcUaBinaryRuntimeContext&& context)
    : runtime_{OpcUaRuntimeContext{
          .executor = context.executor,
          .session_manager = context.session_manager,
          .monitored_item_service = context.monitored_item_service,
          .attribute_service = context.attribute_service,
          .view_service = context.view_service,
          .history_service = context.history_service,
          .method_service = context.method_service,
          .node_management_service = context.node_management_service,
          .now = std::move(context.now),
      }} {}

Awaitable<OpcUaBinaryResponseBody> OpcUaBinaryRuntime::HandleBody(
    OpcUaBinaryConnectionState& connection,
    OpcUaBinaryRequestBody request) {
  co_return co_await runtime_.Handle(connection, std::move(request));
}

void OpcUaBinaryRuntime::Detach(OpcUaBinaryConnectionState& connection) {
  runtime_.Detach(connection);
}

}  // namespace opcua
