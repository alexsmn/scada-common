#include "opcua/opcua_binary_service_dispatcher.h"
#include "opcua/opcua_binary_service_codec.h"

namespace opcua {
namespace {
template <typename T>
constexpr bool kIsSessionRequest =
    std::is_same_v<T, OpcUaBinaryCreateSessionRequest> ||
    std::is_same_v<T, OpcUaBinaryActivateSessionRequest> ||
    std::is_same_v<T, OpcUaBinaryCloseSessionRequest>;

template <typename Response>
std::optional<std::vector<char>> EncodeBinaryResponse(
    scada::UInt32 request_handle,
    Response response) {
  return EncodeOpcUaBinaryServiceResponse(
      request_handle, OpcUaBinaryResponseBody{std::move(response)});
}

template <typename Request>
struct BinaryAuthenticatedRequestTraits;

#define OPCUA_BINARY_AUTHENTICATED_REQUESTS(X)                                  \
  X(OpcUaBinaryBrowseNextRequest, OpcUaBinaryBrowseNextResponse)                \
  X(OpcUaBinaryReadRequest, OpcUaBinaryReadResponse)                            \
  X(OpcUaBinaryBrowseRequest, OpcUaBinaryBrowseResponse)                        \
  X(OpcUaBinaryTranslateBrowsePathsRequest,                                     \
    OpcUaBinaryTranslateBrowsePathsResponse)                                    \
  X(OpcUaBinaryCallRequest, OpcUaBinaryCallResponse)                            \
  X(OpcUaBinaryHistoryReadRawRequest, OpcUaBinaryHistoryReadRawResponse)        \
  X(OpcUaBinaryHistoryReadEventsRequest, OpcUaBinaryHistoryReadEventsResponse)  \
  X(OpcUaBinaryWriteRequest, OpcUaBinaryWriteResponse)                          \
  X(OpcUaBinaryDeleteNodesRequest, OpcUaBinaryDeleteNodesResponse)              \
  X(OpcUaBinaryAddNodesRequest, OpcUaBinaryAddNodesResponse)                    \
  X(OpcUaBinaryDeleteReferencesRequest,                                         \
    OpcUaBinaryDeleteReferencesResponse)                                        \
  X(OpcUaBinaryAddReferencesRequest, OpcUaBinaryAddReferencesResponse)          \
  X(OpcUaBinaryCreateSubscriptionRequest,                                       \
    OpcUaBinaryCreateSubscriptionResponse)                                      \
  X(OpcUaBinaryModifySubscriptionRequest,                                       \
    OpcUaBinaryModifySubscriptionResponse)                                      \
  X(OpcUaBinarySetPublishingModeRequest,                                        \
    OpcUaBinarySetPublishingModeResponse)                                       \
  X(OpcUaBinaryDeleteSubscriptionsRequest,                                      \
    OpcUaBinaryDeleteSubscriptionsResponse)                                     \
  X(OpcUaBinaryCreateMonitoredItemsRequest,                                     \
    OpcUaBinaryCreateMonitoredItemsResponse)                                    \
  X(OpcUaBinaryModifyMonitoredItemsRequest,                                     \
    OpcUaBinaryModifyMonitoredItemsResponse)                                    \
  X(OpcUaBinaryPublishRequest, OpcUaBinaryPublishResponse)                      \
  X(OpcUaBinaryRepublishRequest, OpcUaBinaryRepublishResponse)                  \
  X(OpcUaBinaryTransferSubscriptionsRequest,                                    \
    OpcUaBinaryTransferSubscriptionsResponse)                                   \
  X(OpcUaBinaryDeleteMonitoredItemsRequest,                                     \
    OpcUaBinaryDeleteMonitoredItemsResponse)                                    \
  X(OpcUaBinarySetMonitoringModeRequest,                                        \
    OpcUaBinarySetMonitoringModeResponse)

#define OPCUA_BINARY_DECLARE_AUTH_TRAITS(Request, Response) \
  template <>                                               \
  struct BinaryAuthenticatedRequestTraits<Request> {        \
    using ResponseType = Response;                          \
  };
OPCUA_BINARY_AUTHENTICATED_REQUESTS(OPCUA_BINARY_DECLARE_AUTH_TRAITS)
#undef OPCUA_BINARY_DECLARE_AUTH_TRAITS

template <typename Request>
using BinaryAuthenticatedResponse =
    typename BinaryAuthenticatedRequestTraits<Request>::ResponseType;

template <typename Request>
concept BinaryAuthenticatedRequest =
    requires { typename BinaryAuthenticatedResponse<Request>; };

template <typename Response>
auto MakeBinaryResponseEncoder(const OpcUaBinaryDecodedRequest&) {
  return [](scada::UInt32 request_handle, Response response) {
    return EncodeBinaryResponse(request_handle, std::move(response));
  };
}

template <>
auto MakeBinaryResponseEncoder<OpcUaBinaryHistoryReadEventsResponse>(
    const OpcUaBinaryDecodedRequest& request) {
  return [field_paths = request.history_event_field_paths](
             scada::UInt32 request_handle,
             OpcUaBinaryHistoryReadEventsResponse response) {
    return EncodeOpcUaBinaryHistoryReadEventsResponse(
        request_handle, response, field_paths);
  };
}

#undef OPCUA_BINARY_AUTHENTICATED_REQUESTS
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
  co_return EncodeBinaryResponse(request_handle, std::move(response));
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
  co_return EncodeBinaryResponse(header.request_handle, std::move(response));
}

Awaitable<std::optional<std::vector<char>>>
OpcUaBinaryServiceDispatcher::HandleSessionRequest(
    const OpcUaBinaryServiceRequestHeader& header,
    OpcUaBinaryCloseSessionRequest request) {
  const auto session = session_manager_.FindSession(header.authentication_token);
  if (!session.has_value()) {
    co_return EncodeBinaryResponse(
        header.request_handle,
        OpcUaBinaryCloseSessionResponse{
            .status = scada::StatusCode::Bad_SessionIsLoggedOff});
  }

  request.session_id = session->session_id;
  request.authentication_token = header.authentication_token;
  auto response = co_await runtime_.Handle<OpcUaBinaryCloseSessionResponse>(
      connection_, std::move(request));
  co_return EncodeBinaryResponse(header.request_handle, std::move(response));
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
        } else if constexpr (BinaryAuthenticatedRequest<T>) {
          co_return co_await
              HandleAuthenticatedRequest<BinaryAuthenticatedResponse<T>>(
                  *request, std::move(typed_request),
                  MakeBinaryResponseEncoder<BinaryAuthenticatedResponse<T>>(
                      *request));
        } else {
          static_assert(!kIsSessionRequest<T>,
                        "Session requests must be handled above");
          co_return std::nullopt;
        }
      },
      request->body);
}

}  // namespace opcua
