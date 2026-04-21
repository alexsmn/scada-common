#pragma once

#include "base/awaitable.h"
#include "opcua/opcua_binary_message.h"
#include "opcua_ws/opcua_ws_runtime.h"

namespace opcua {

using OpcUaBinaryConnectionState = opcua_ws::OpcUaWsConnectionState;

struct OpcUaBinaryRuntimeContext {
  std::shared_ptr<Executor> executor;
  opcua_ws::OpcUaWsSessionManager& session_manager;
  scada::MonitoredItemService& monitored_item_service;
  scada::AttributeService& attribute_service;
  scada::ViewService& view_service;
  scada::HistoryService& history_service;
  scada::MethodService& method_service;
  scada::NodeManagementService& node_management_service;
  std::function<base::Time()> now = &base::Time::Now;
};

// First in-repo UA Binary adapter step: reuse the OPC UA WS runtime as the
// canonical server-side session/subscription/service core while the actual UA
// Binary wire protocol layer is implemented separately.
class OpcUaBinaryRuntime {
 public:
  explicit OpcUaBinaryRuntime(OpcUaBinaryRuntimeContext&& context);

  [[nodiscard]] Awaitable<OpcUaBinaryCreateSessionResponse> CreateSession(
      OpcUaBinaryCreateSessionRequest request = {});
  [[nodiscard]] Awaitable<OpcUaBinaryActivateSessionResponse> ActivateSession(
      OpcUaBinaryConnectionState& connection,
      OpcUaBinaryActivateSessionRequest request);
  [[nodiscard]] Awaitable<OpcUaBinaryCloseSessionResponse> CloseSession(
      OpcUaBinaryConnectionState& connection,
      OpcUaBinaryCloseSessionRequest request);

  template <typename Response, typename Request>
  [[nodiscard]] Awaitable<Response> Handle(OpcUaBinaryConnectionState& connection,
                                           Request request) {
    auto body = co_await HandleBody(connection, OpcUaBinaryRequestBody{
                                                    std::move(request)});
    if (auto* typed = std::get_if<Response>(&body)) {
      co_return std::move(*typed);
    }
    if (auto* fault = std::get_if<OpcUaBinaryServiceFault>(&body)) {
      co_return Response{.status = fault->status};
    }
    co_return Response{.status = scada::StatusCode::Bad};
  }

  void Detach(OpcUaBinaryConnectionState& connection);

 private:
  [[nodiscard]] Awaitable<OpcUaBinaryResponseBody> HandleBody(
      OpcUaBinaryConnectionState& connection,
      OpcUaBinaryRequestBody request);

  opcua_ws::OpcUaWsSessionManager& session_manager_;
  opcua_ws::OpcUaWsRuntime runtime_;
};

}  // namespace opcua
