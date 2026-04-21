#include "opcua/opcua_binary_runtime.h"

namespace opcua {

OpcUaBinaryRuntime::OpcUaBinaryRuntime(OpcUaBinaryRuntimeContext&& context)
    : session_manager_{context.session_manager},
      runtime_{opcua_ws::OpcUaWsRuntimeContext{
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

Awaitable<OpcUaBinaryCreateSessionResponse> OpcUaBinaryRuntime::CreateSession(
    OpcUaBinaryCreateSessionRequest request) {
  co_return co_await session_manager_.CreateSession(std::move(request));
}

Awaitable<OpcUaBinaryActivateSessionResponse> OpcUaBinaryRuntime::ActivateSession(
    OpcUaBinaryConnectionState& connection,
    OpcUaBinaryActivateSessionRequest request) {
  auto body = co_await HandleBody(connection,
                                  OpcUaBinaryRequestBody{std::move(request)});
  if (auto* typed = std::get_if<OpcUaBinaryActivateSessionResponse>(&body)) {
    co_return std::move(*typed);
  }
  if (auto* fault = std::get_if<OpcUaBinaryServiceFault>(&body)) {
    co_return OpcUaBinaryActivateSessionResponse{.status = fault->status};
  }
  co_return OpcUaBinaryActivateSessionResponse{.status = scada::StatusCode::Bad};
}

Awaitable<OpcUaBinaryCloseSessionResponse> OpcUaBinaryRuntime::CloseSession(
    OpcUaBinaryConnectionState& connection,
    OpcUaBinaryCloseSessionRequest request) {
  auto body = co_await HandleBody(connection,
                                  OpcUaBinaryRequestBody{std::move(request)});
  if (auto* typed = std::get_if<OpcUaBinaryCloseSessionResponse>(&body)) {
    co_return std::move(*typed);
  }
  if (auto* fault = std::get_if<OpcUaBinaryServiceFault>(&body)) {
    co_return OpcUaBinaryCloseSessionResponse{.status = fault->status};
  }
  co_return OpcUaBinaryCloseSessionResponse{.status = scada::StatusCode::Bad};
}

Awaitable<OpcUaBinaryResponseBody> OpcUaBinaryRuntime::HandleBody(
    OpcUaBinaryConnectionState& connection,
    OpcUaBinaryRequestBody request) {
  auto response =
      co_await runtime_.Handle(connection,
                               {.request_handle = 0, .body = std::move(request)});
  co_return std::move(response.body);
}

void OpcUaBinaryRuntime::Detach(OpcUaBinaryConnectionState& connection) {
  runtime_.Detach(connection);
}

}  // namespace opcua
