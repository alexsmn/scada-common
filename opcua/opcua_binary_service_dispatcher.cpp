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
    const OpcUaBinaryHistoryReadRawRequest& typed_request) {
  co_return co_await
      HandleAuthenticatedRequest<OpcUaBinaryHistoryReadRawResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryHistoryReadEventsRequest& typed_request) {
  if (!connection_.authentication_token.has_value() ||
      *connection_.authentication_token != request.header.authentication_token) {
    co_return EncodeOpcUaBinaryHistoryReadEventsResponse(
        request.header.request_handle,
        BuildBinaryRuntimeErrorResponse<OpcUaBinaryHistoryReadEventsResponse>(
            scada::StatusCode::Bad_SessionIsLoggedOff),
        request.history_event_field_paths);
  }

  const auto response = co_await runtime_.Handle<OpcUaBinaryHistoryReadEventsResponse>(
      connection_, typed_request);
  co_return EncodeOpcUaBinaryHistoryReadEventsResponse(
      request.header.request_handle, response, request.history_event_field_paths);
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

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryCreateSubscriptionRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinaryCreateSubscriptionResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryModifySubscriptionRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinaryModifySubscriptionResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinarySetPublishingModeRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinarySetPublishingModeResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryDeleteSubscriptionsRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinaryDeleteSubscriptionsResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryCreateMonitoredItemsRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinaryCreateMonitoredItemsResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryModifyMonitoredItemsRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinaryModifyMonitoredItemsResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryPublishRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryPublishResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryRepublishRequest& typed_request) {
  co_return co_await HandleAuthenticatedRequest<OpcUaBinaryRepublishResponse>(
      request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryTransferSubscriptionsRequest& typed_request) {
  co_return co_await
      HandleAuthenticatedRequest<OpcUaBinaryTransferSubscriptionsResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinaryDeleteMonitoredItemsRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinaryDeleteMonitoredItemsResponse>(
          request.header, typed_request);
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleRequest(
    const OpcUaBinaryDecodedRequest& request,
    const OpcUaBinarySetMonitoringModeRequest& typed_request) {
  co_return
      co_await HandleAuthenticatedRequest<OpcUaBinarySetMonitoringModeResponse>(
          request.header, typed_request);
}

}  // namespace opcua
