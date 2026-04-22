#include "opcua/opcua_binary_service_dispatcher.h"
#include "opcua/opcua_binary_service_codec.h"

namespace opcua {
namespace {

}  // namespace

OpcUaBinaryServiceDispatcher::OpcUaBinaryServiceDispatcher(Context context)
    : runtime_{context.runtime},
      session_manager_{context.session_manager},
      connection_{context.connection},
      session_service_{{.runtime = context.runtime,
                        .session_manager = context.session_manager,
                        .connection = context.connection}} {}

Awaitable<std::optional<std::vector<char>>> OpcUaBinaryServiceDispatcher::HandlePayload(
    std::vector<char> payload) {
  const auto request = DecodeOpcUaBinaryServiceRequest(payload);
  if (!request.has_value()) {
    co_return std::nullopt;
  }

  if (std::holds_alternative<OpcUaBinaryCreateSessionRequest>(request->body) ||
      std::holds_alternative<OpcUaBinaryActivateSessionRequest>(request->body) ||
      std::holds_alternative<OpcUaBinaryCloseSessionRequest>(request->body)) {
    co_return co_await session_service_.HandleRequest(*request);
  }
  if (std::holds_alternative<OpcUaBinaryBrowseRequest>(request->body)) {
    co_return co_await HandleBrowseRequest(*request);
  }
  if (std::holds_alternative<OpcUaBinaryTranslateBrowsePathsRequest>(
          request->body)) {
    co_return co_await HandleTranslateBrowsePathsRequest(*request);
  }
  if (std::holds_alternative<OpcUaBinaryCallRequest>(request->body)) {
    co_return co_await HandleCallRequest(*request);
  }
  if (std::holds_alternative<OpcUaBinaryReadRequest>(request->body)) {
    co_return co_await HandleReadRequest(*request);
  }
  if (std::holds_alternative<OpcUaBinaryWriteRequest>(request->body)) {
    co_return co_await HandleWriteRequest(*request);
  }
  if (std::holds_alternative<OpcUaBinaryDeleteNodesRequest>(request->body)) {
    co_return co_await HandleDeleteNodesRequest(*request);
  }
  co_return std::nullopt;
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleBrowseRequest(
    const OpcUaBinaryDecodedRequest& request) {
  const auto* browse = std::get_if<OpcUaBinaryBrowseRequest>(&request.body);
  if (browse == nullptr) {
    co_return std::nullopt;
  }
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeOpcUaBinaryServiceResponse(
        request.header.request_handle,
        OpcUaBinaryResponseBody{OpcUaBinaryBrowseResponse{
            .status = scada::StatusCode::Bad_SessionIsLoggedOff}});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryBrowseResponse>(connection_, *browse);
  co_return EncodeOpcUaBinaryServiceResponse(request.header.request_handle,
                                             OpcUaBinaryResponseBody{response});
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleReadRequest(
    const OpcUaBinaryDecodedRequest& request) {
  const auto* read = std::get_if<OpcUaBinaryReadRequest>(&request.body);
  if (read == nullptr) {
    co_return std::nullopt;
  }
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeOpcUaBinaryServiceResponse(
        request.header.request_handle,
        OpcUaBinaryResponseBody{OpcUaBinaryReadResponse{
            .status = scada::StatusCode::Bad_SessionIsLoggedOff}});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryReadResponse>(connection_, *read);
  co_return EncodeOpcUaBinaryServiceResponse(request.header.request_handle,
                                             OpcUaBinaryResponseBody{response});
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleTranslateBrowsePathsRequest(
    const OpcUaBinaryDecodedRequest& request) {
  const auto* translate =
      std::get_if<OpcUaBinaryTranslateBrowsePathsRequest>(&request.body);
  if (translate == nullptr) {
    co_return std::nullopt;
  }
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeOpcUaBinaryServiceResponse(
        request.header.request_handle,
        OpcUaBinaryResponseBody{OpcUaBinaryTranslateBrowsePathsResponse{
            .status = scada::StatusCode::Bad_SessionIsLoggedOff}});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryTranslateBrowsePathsResponse>(
          connection_, *translate);
  co_return EncodeOpcUaBinaryServiceResponse(request.header.request_handle,
                                             OpcUaBinaryResponseBody{response});
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleCallRequest(
    const OpcUaBinaryDecodedRequest& request) {
  const auto* call = std::get_if<OpcUaBinaryCallRequest>(&request.body);
  if (call == nullptr) {
    co_return std::nullopt;
  }
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeOpcUaBinaryServiceResponse(
        request.header.request_handle,
        OpcUaBinaryResponseBody{
            OpcUaBinaryCallResponse{
                .results = {{.status = scada::StatusCode::Bad_SessionIsLoggedOff}}}});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryCallResponse>(connection_, *call);
  co_return EncodeOpcUaBinaryServiceResponse(request.header.request_handle,
                                             OpcUaBinaryResponseBody{response});
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleWriteRequest(
    const OpcUaBinaryDecodedRequest& request) {
  const auto* write = std::get_if<OpcUaBinaryWriteRequest>(&request.body);
  if (write == nullptr) {
    co_return std::nullopt;
  }
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeOpcUaBinaryServiceResponse(
        request.header.request_handle,
        OpcUaBinaryResponseBody{OpcUaBinaryWriteResponse{
            .status = scada::StatusCode::Bad_SessionIsLoggedOff}});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryWriteResponse>(connection_, *write);
  co_return EncodeOpcUaBinaryServiceResponse(request.header.request_handle,
                                             OpcUaBinaryResponseBody{response});
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleDeleteNodesRequest(
    const OpcUaBinaryDecodedRequest& request) {
  const auto* delete_nodes =
      std::get_if<OpcUaBinaryDeleteNodesRequest>(&request.body);
  if (delete_nodes == nullptr) {
    co_return std::nullopt;
  }
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeOpcUaBinaryServiceResponse(
        request.header.request_handle,
        OpcUaBinaryResponseBody{OpcUaBinaryDeleteNodesResponse{
            .status = scada::StatusCode::Bad_SessionIsLoggedOff}});
  }

  const auto response =
      co_await runtime_.Handle<OpcUaBinaryDeleteNodesResponse>(connection_,
                                                               *delete_nodes);
  co_return EncodeOpcUaBinaryServiceResponse(request.header.request_handle,
                                             OpcUaBinaryResponseBody{response});
}

}  // namespace opcua
