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

  co_return co_await std::visit(
      [this, &request](const auto& typed_request)
          -> Awaitable<std::optional<std::vector<char>>> {
        co_return co_await HandleRequest(*request, typed_request);
      },
      request->body);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryBrowseNextRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryBrowseNextResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryReadRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryReadResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryBrowseRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryBrowseResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryTranslateBrowsePathsRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinaryTranslateBrowsePathsResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryCallRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryCallResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryWriteRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryWriteResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryDeleteNodesRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryDeleteNodesResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryAddNodesRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryAddNodesResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryDeleteReferencesRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinaryDeleteReferencesResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryAddReferencesRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryAddReferencesResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryCreateSessionRequest&) {
  co_return co_await session_service_.HandleRequest(request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryActivateSessionRequest&) {
  co_return co_await session_service_.HandleRequest(request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryCloseSessionRequest&) {
  co_return co_await session_service_.HandleRequest(request);
}

}  // namespace opcua
