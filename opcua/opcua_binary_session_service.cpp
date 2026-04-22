#include "opcua/opcua_binary_session_service.h"
#include "opcua/opcua_binary_service_codec.h"

namespace opcua {

OpcUaBinarySessionService::OpcUaBinarySessionService(Context context)
    : runtime_{context.runtime},
      session_manager_{context.session_manager},
      connection_{context.connection} {}

Awaitable<std::optional<std::vector<char>>> OpcUaBinarySessionService::HandleRequest(
    scada::UInt32 request_handle,
    OpcUaBinaryCreateSessionRequest request) {
  auto response = co_await runtime_.CreateSession(std::move(request));
  co_return EncodeOpcUaBinaryServiceResponse(request_handle,
                                             OpcUaBinaryResponseBody{response});
}

Awaitable<std::optional<std::vector<char>>> OpcUaBinarySessionService::HandleRequest(
    const OpcUaBinaryServiceRequestHeader& header,
    OpcUaBinaryActivateSessionRequest request) {
  const auto session = session_manager_.FindSession(header.authentication_token);
  if (!session.has_value()) {
    co_return std::nullopt;
  }

  request.session_id = session->session_id;
  request.authentication_token = header.authentication_token;
  auto response = co_await runtime_.ActivateSession(connection_, std::move(request));
  co_return EncodeOpcUaBinaryServiceResponse(header.request_handle,
                                             OpcUaBinaryResponseBody{response});
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinarySessionService::HandleCloseRequest(
    scada::UInt32 request_handle,
    const scada::NodeId& authentication_token) {
  const auto session = session_manager_.FindSession(authentication_token);
  if (!session.has_value()) {
    co_return EncodeOpcUaBinaryServiceResponse(
        request_handle,
        OpcUaBinaryResponseBody{OpcUaBinaryCloseSessionResponse{
            .status = scada::StatusCode::Bad_SessionIsLoggedOff}});
  }

  auto response = co_await runtime_.CloseSession(
      connection_,
      {.session_id = session->session_id,
       .authentication_token = authentication_token});
  co_return EncodeOpcUaBinaryServiceResponse(request_handle,
                                             OpcUaBinaryResponseBody{response});
}

}  // namespace opcua
