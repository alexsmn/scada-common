#include "opcua/opcua_binary_runtime.h"

namespace opcua {
namespace {
template <typename T>
constexpr bool kIsBinarySessionRequest =
    std::is_same_v<T, OpcUaBinaryCreateSessionRequest> ||
    std::is_same_v<T, OpcUaBinaryActivateSessionRequest> ||
    std::is_same_v<T, OpcUaBinaryCloseSessionRequest>;

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

#undef OPCUA_BINARY_AUTHENTICATED_REQUESTS
}  // namespace

OpcUaBinaryRuntime::OpcUaBinaryRuntime(OpcUaBinaryRuntimeContext&& context)
    : session_manager_{context.session_manager},
      runtime_{OpcUaRuntimeContext{
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

Awaitable<std::optional<OpcUaBinaryResponseBody>>
OpcUaBinaryRuntime::HandleSessionRequest(
    OpcUaBinaryConnectionState& connection,
    OpcUaBinaryCreateSessionRequest request) {
  co_return OpcUaBinaryResponseBody{
      co_await Handle<OpcUaBinaryCreateSessionResponse>(connection,
                                                        std::move(request))};
}

Awaitable<std::optional<OpcUaBinaryResponseBody>>
OpcUaBinaryRuntime::HandleSessionRequest(
    OpcUaBinaryConnectionState& connection,
    const OpcUaBinaryServiceRequestHeader& header,
    OpcUaBinaryActivateSessionRequest request) {
  const auto session = session_manager_.FindSession(header.authentication_token);
  if (!session.has_value()) {
    co_return std::nullopt;
  }

  request.session_id = session->session_id;
  request.authentication_token = header.authentication_token;
  co_return OpcUaBinaryResponseBody{
      co_await Handle<OpcUaBinaryActivateSessionResponse>(connection,
                                                          std::move(request))};
}

Awaitable<std::optional<OpcUaBinaryResponseBody>>
OpcUaBinaryRuntime::HandleSessionRequest(
    OpcUaBinaryConnectionState& connection,
    const OpcUaBinaryServiceRequestHeader& header,
    OpcUaBinaryCloseSessionRequest request) {
  const auto session = session_manager_.FindSession(header.authentication_token);
  if (!session.has_value()) {
    co_return OpcUaBinaryResponseBody{OpcUaBinaryCloseSessionResponse{
        .status = scada::StatusCode::Bad_SessionIsLoggedOff}};
  }

  request.session_id = session->session_id;
  request.authentication_token = header.authentication_token;
  co_return OpcUaBinaryResponseBody{
      co_await Handle<OpcUaBinaryCloseSessionResponse>(connection,
                                                       std::move(request))};
}

Awaitable<std::optional<OpcUaBinaryResponseBody>>
OpcUaBinaryRuntime::HandleDecodedRequest(
    OpcUaBinaryConnectionState& connection,
    const OpcUaBinaryDecodedRequest& request) {
  co_return co_await std::visit(
      [this, &connection, &request](auto typed_request)
          -> Awaitable<std::optional<OpcUaBinaryResponseBody>> {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, OpcUaBinaryCreateSessionRequest>) {
          co_return co_await HandleSessionRequest(connection,
                                                  std::move(typed_request));
        } else if constexpr (std::is_same_v<T,
                                            OpcUaBinaryActivateSessionRequest>) {
          co_return co_await HandleSessionRequest(connection, request.header,
                                                  std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaBinaryCloseSessionRequest>) {
          co_return co_await HandleSessionRequest(connection, request.header,
                                                  std::move(typed_request));
        } else if constexpr (BinaryAuthenticatedRequest<T>) {
          co_return co_await HandleAuthenticatedRequest<
              BinaryAuthenticatedResponse<T>>(connection, request,
                                              std::move(typed_request));
        } else {
          static_assert(!kIsBinarySessionRequest<T>,
                        "Session requests must be handled above");
          co_return std::nullopt;
        }
      },
      request.body);
}

}  // namespace opcua
