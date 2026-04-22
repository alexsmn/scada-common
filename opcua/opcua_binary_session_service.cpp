#include "opcua/opcua_binary_session_service.h"
#include "opcua/opcua_binary_service_codec.h"

namespace opcua {

OpcUaBinarySessionService::OpcUaBinarySessionService(Context context)
    : runtime_{context.runtime},
      session_manager_{context.session_manager},
      connection_{context.connection} {}

Awaitable<std::optional<std::vector<char>>> OpcUaBinarySessionService::HandleRequest(
    OpcUaBinaryDecodedRequest request) {
  if (const auto* create =
          std::get_if<OpcUaBinaryCreateSessionRequest>(&request.body)) {
    auto response = co_await runtime_.CreateSession(*create);
    co_return EncodeOpcUaBinaryServiceResponse(request.header.request_handle,
                                               OpcUaBinaryResponseBody{response});
  }

  if (const auto* activate =
          std::get_if<OpcUaBinaryActivateSessionRequest>(&request.body)) {
    auto runtime_request = *activate;
    const auto session =
        session_manager_.FindSession(request.header.authentication_token);
    if (!session.has_value()) {
      co_return std::nullopt;
    }
    runtime_request.session_id = session->session_id;
    runtime_request.authentication_token = request.header.authentication_token;
    auto response =
        co_await runtime_.ActivateSession(connection_, std::move(runtime_request));
    co_return EncodeOpcUaBinaryServiceResponse(request.header.request_handle,
                                               OpcUaBinaryResponseBody{response});
  }

  if (std::holds_alternative<OpcUaBinaryCloseSessionRequest>(request.body)) {
    const auto session =
        session_manager_.FindSession(request.header.authentication_token);
    if (!session.has_value()) {
      co_return EncodeOpcUaBinaryServiceResponse(
          request.header.request_handle,
          OpcUaBinaryResponseBody{OpcUaBinaryCloseSessionResponse{
              .status = scada::StatusCode::Bad_SessionIsLoggedOff}});
    }
    auto response = co_await runtime_.CloseSession(
        connection_,
        {.session_id = session->session_id,
         .authentication_token = request.header.authentication_token});
    co_return EncodeOpcUaBinaryServiceResponse(request.header.request_handle,
                                               OpcUaBinaryResponseBody{response});
  }

  co_return std::nullopt;
}

}  // namespace opcua
