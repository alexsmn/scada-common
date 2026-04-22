#include "opcua/opcua_binary_service_dispatcher.h"
#include "opcua/opcua_binary_service_codec.h"

namespace opcua {
namespace {
template <typename T>
constexpr bool kIsSessionRequest =
    std::is_same_v<T, OpcUaBinaryCreateSessionRequest> ||
    std::is_same_v<T, OpcUaBinaryActivateSessionRequest> ||
    std::is_same_v<T, OpcUaBinaryCloseSessionRequest>;

}  // namespace

OpcUaBinaryServiceDispatcher::OpcUaBinaryServiceDispatcher(Context context)
    : runtime_{context.runtime},
      session_manager_{context.session_manager},
      connection_{context.connection} {}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleSessionRequest(
    scada::UInt32 request_handle,
    OpcUaBinaryCreateSessionRequest request) {
  auto response = co_await runtime_.Handle<OpcUaBinaryCreateSessionResponse>(
      connection_, std::move(request));
  co_return EncodeResponse(request_handle, std::move(response));
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleSessionRequest(
    const OpcUaBinaryServiceRequestHeader& header,
    OpcUaBinaryActivateSessionRequest request) {
  const auto session = session_manager_.FindSession(header.authentication_token);
  if (!session.has_value()) {
    co_return std::nullopt;
  }

  request.session_id = session->session_id;
  request.authentication_token = header.authentication_token;
  auto response = co_await runtime_.Handle<OpcUaBinaryActivateSessionResponse>(
      connection_, std::move(request));
  co_return EncodeResponse(header.request_handle, std::move(response));
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleSessionRequest(
    const OpcUaBinaryServiceRequestHeader& header,
    OpcUaBinaryCloseSessionRequest request) {
  const auto session = session_manager_.FindSession(header.authentication_token);
  if (!session.has_value()) {
    co_return EncodeResponse(
        header.request_handle,
        OpcUaBinaryCloseSessionResponse{
            .status = scada::StatusCode::Bad_SessionIsLoggedOff});
  }

  request.session_id = session->session_id;
  request.authentication_token = header.authentication_token;
  auto response = co_await runtime_.Handle<OpcUaBinaryCloseSessionResponse>(
      connection_, std::move(request));
  co_return EncodeResponse(header.request_handle, std::move(response));
}

Awaitable<std::optional<std::vector<char>>> OpcUaBinaryServiceDispatcher::HandlePayload(
    std::vector<char> payload) {
  const auto request = DecodeOpcUaBinaryServiceRequest(payload);
  if (!request.has_value()) {
    co_return std::nullopt;
  }

  co_return co_await std::visit(
      [this, &request](auto typed_request)
          -> Awaitable<std::optional<std::vector<char>>> {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, OpcUaBinaryCreateSessionRequest>) {
          co_return co_await HandleSessionRequest(
              request->header.request_handle, std::move(typed_request));
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryActivateSessionRequest>) {
          co_return co_await HandleSessionRequest(
              request->header, std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryCloseSessionRequest>) {
          co_return co_await HandleSessionRequest(request->header,
                                                  std::move(typed_request));
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryBrowseNextRequest>) {
          co_return co_await
              HandleAuthenticatedRequest<OpcUaBinaryBrowseNextResponse>(
                  *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryReadRequest>) {
          co_return co_await HandleAuthenticatedRequest<OpcUaBinaryReadResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryBrowseRequest>) {
          co_return co_await
              HandleAuthenticatedRequest<OpcUaBinaryBrowseResponse>(
                  *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<
                                 T, OpcUaBinaryTranslateBrowsePathsRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryTranslateBrowsePathsResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryCallRequest>) {
          co_return co_await HandleAuthenticatedRequest<OpcUaBinaryCallResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryHistoryReadRawRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryHistoryReadRawResponse>(*request,
                                                 std::move(typed_request));
        } else if constexpr (std::is_same_v<
                                 T, OpcUaBinaryHistoryReadEventsRequest>) {
          auto encode = [field_paths = request->history_event_field_paths](
                            scada::UInt32 request_handle,
                            OpcUaBinaryHistoryReadEventsResponse response) {
            return EncodeOpcUaBinaryHistoryReadEventsResponse(
                request_handle, response, field_paths);
          };
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryHistoryReadEventsResponse>(
              *request, std::move(typed_request), std::move(encode));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryWriteRequest>) {
          co_return co_await
              HandleAuthenticatedRequest<OpcUaBinaryWriteResponse>(
                  *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryDeleteNodesRequest>) {
          co_return co_await
              HandleAuthenticatedRequest<OpcUaBinaryDeleteNodesResponse>(
                  *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryAddNodesRequest>) {
          co_return co_await
              HandleAuthenticatedRequest<OpcUaBinaryAddNodesResponse>(
                  *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryDeleteReferencesRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryDeleteReferencesResponse>(*request,
                                                   std::move(typed_request));
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryAddReferencesRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryAddReferencesResponse>(*request,
                                                std::move(typed_request));
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryCreateSubscriptionRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryCreateSubscriptionResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryModifySubscriptionRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryModifySubscriptionResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<
                                 T, OpcUaBinarySetPublishingModeRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinarySetPublishingModeResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<
                                 T, OpcUaBinaryDeleteSubscriptionsRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryDeleteSubscriptionsResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<
                                 T, OpcUaBinaryCreateMonitoredItemsRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryCreateMonitoredItemsResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<
                                 T, OpcUaBinaryModifyMonitoredItemsRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryModifyMonitoredItemsResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryPublishRequest>) {
          co_return co_await
              HandleAuthenticatedRequest<OpcUaBinaryPublishResponse>(
                  *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryRepublishRequest>) {
          co_return co_await
              HandleAuthenticatedRequest<OpcUaBinaryRepublishResponse>(
                  *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<
                                 T, OpcUaBinaryTransferSubscriptionsRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryTransferSubscriptionsResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<
                                 T, OpcUaBinaryDeleteMonitoredItemsRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinaryDeleteMonitoredItemsResponse>(
              *request, std::move(typed_request));
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinarySetMonitoringModeRequest>) {
          co_return co_await HandleAuthenticatedRequest<
              OpcUaBinarySetMonitoringModeResponse>(
              *request, std::move(typed_request));
        } else {
          static_assert(!kIsSessionRequest<T>,
                        "Session requests must be handled above");
          co_return std::nullopt;
        }
      },
      request->body);
}

}  // namespace opcua
