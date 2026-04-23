#include "opcua/opcua_binary_runtime.h"

namespace opcua {
namespace {
template <typename T>
constexpr bool kIsBinarySessionRequest =
    std::is_same_v<T, OpcUaCreateSessionRequest> ||
    std::is_same_v<T, OpcUaActivateSessionRequest> ||
    std::is_same_v<T, OpcUaCloseSessionRequest>;

template <typename Request>
struct BinaryAuthenticatedRequestTraits;

#define OPCUA_BINARY_AUTHENTICATED_REQUESTS(X)                                  \
  X(BrowseNextRequest, BrowseNextResponse)                                      \
  X(ReadRequest, ReadResponse)                                                  \
  X(BrowseRequest, BrowseResponse)                                              \
  X(TranslateBrowsePathsRequest, TranslateBrowsePathsResponse)                  \
  X(CallRequest, CallResponse)                                                  \
  X(HistoryReadRawRequest, HistoryReadRawResponse)                              \
  X(HistoryReadEventsRequest, HistoryReadEventsResponse)                        \
  X(WriteRequest, WriteResponse)                                                \
  X(DeleteNodesRequest, DeleteNodesResponse)                                    \
  X(AddNodesRequest, AddNodesResponse)                                          \
  X(DeleteReferencesRequest, DeleteReferencesResponse)                          \
  X(AddReferencesRequest, AddReferencesResponse)                                \
  X(OpcUaCreateSubscriptionRequest, OpcUaCreateSubscriptionResponse)            \
  X(OpcUaModifySubscriptionRequest, OpcUaModifySubscriptionResponse)            \
  X(OpcUaSetPublishingModeRequest, OpcUaSetPublishingModeResponse)              \
  X(OpcUaDeleteSubscriptionsRequest, OpcUaDeleteSubscriptionsResponse)          \
  X(OpcUaCreateMonitoredItemsRequest, OpcUaCreateMonitoredItemsResponse)        \
  X(OpcUaModifyMonitoredItemsRequest, OpcUaModifyMonitoredItemsResponse)        \
  X(OpcUaPublishRequest, OpcUaPublishResponse)                                  \
  X(OpcUaRepublishRequest, OpcUaRepublishResponse)                              \
  X(OpcUaTransferSubscriptionsRequest, OpcUaTransferSubscriptionsResponse)      \
  X(OpcUaDeleteMonitoredItemsRequest, OpcUaDeleteMonitoredItemsResponse)        \
  X(OpcUaSetMonitoringModeRequest, OpcUaSetMonitoringModeResponse)

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

Awaitable<OpcUaResponseBody> OpcUaBinaryRuntime::HandleBody(
    OpcUaBinaryConnectionState& connection,
    OpcUaRequestBody request) {
  co_return co_await runtime_.Handle(connection, std::move(request));
}

void OpcUaBinaryRuntime::Detach(OpcUaBinaryConnectionState& connection) {
  runtime_.Detach(connection);
}

Awaitable<std::optional<OpcUaBinaryResponseBody>>
OpcUaBinaryRuntime::HandleSessionRequest(
    OpcUaBinaryConnectionState& connection,
    OpcUaCreateSessionRequest request) {
  co_return OpcUaBinaryResponseBody{
      co_await Handle<OpcUaCreateSessionResponse>(connection, std::move(request))};
}

Awaitable<std::optional<OpcUaBinaryResponseBody>>
OpcUaBinaryRuntime::HandleSessionRequest(
    OpcUaBinaryConnectionState& connection,
    const OpcUaBinaryServiceRequestHeader& header,
    OpcUaActivateSessionRequest request) {
  const auto session = session_manager_.FindSession(header.authentication_token);
  if (!session.has_value()) {
    co_return std::nullopt;
  }

  request.session_id = session->session_id;
  request.authentication_token = header.authentication_token;
  co_return OpcUaBinaryResponseBody{
      co_await Handle<OpcUaActivateSessionResponse>(connection,
                                                    std::move(request))};
}

Awaitable<std::optional<OpcUaBinaryResponseBody>>
OpcUaBinaryRuntime::HandleSessionRequest(
    OpcUaBinaryConnectionState& connection,
    const OpcUaBinaryServiceRequestHeader& header,
    OpcUaCloseSessionRequest request) {
  const auto session = session_manager_.FindSession(header.authentication_token);
  if (!session.has_value()) {
    co_return OpcUaBinaryResponseBody{OpcUaCloseSessionResponse{
        .status = scada::StatusCode::Bad_SessionIsLoggedOff}};
  }

  request.session_id = session->session_id;
  request.authentication_token = header.authentication_token;
  co_return OpcUaBinaryResponseBody{
      co_await Handle<OpcUaCloseSessionResponse>(connection, std::move(request))};
}

Awaitable<std::optional<OpcUaBinaryResponseBody>>
OpcUaBinaryRuntime::HandleDecodedRequest(
    OpcUaBinaryConnectionState& connection,
    const OpcUaBinaryDecodedRequest& request) {
  co_return co_await std::visit(
      [this, &connection, &request](auto typed_request)
          -> Awaitable<std::optional<OpcUaBinaryResponseBody>> {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, OpcUaCreateSessionRequest>) {
          co_return co_await HandleSessionRequest(connection,
                                                  std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaActivateSessionRequest>) {
          co_return co_await HandleSessionRequest(connection, request.header,
                                                  std::move(typed_request));
        } else if constexpr (std::is_same_v<T, OpcUaCloseSessionRequest>) {
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
