#pragma once

#include "base/awaitable.h"
#include "opcua/opcua_binary_service_codec.h"
#include "opcua/opcua_binary_runtime.h"
#include "opcua/opcua_binary_session_service.h"
#include "opcua_ws/opcua_ws_session_manager.h"

namespace opcua {

class OpcUaBinaryServiceDispatcher {
 public:
  struct Context {
    OpcUaBinaryRuntime& runtime;
    opcua_ws::OpcUaWsSessionManager& session_manager;
    OpcUaBinaryConnectionState& connection;
  };

 explicit OpcUaBinaryServiceDispatcher(Context context);

  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandlePayload(
      std::vector<char> payload);

 private:
  template <typename Response, typename Request>
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleAuthenticatedRequest(const OpcUaBinaryServiceRequestHeader& header,
                             const Request& typed_request) {
    if (!connection_.authentication_token.has_value() ||
        *connection_.authentication_token != header.authentication_token) {
      co_return EncodeOpcUaBinaryServiceResponse(
          header.request_handle,
          OpcUaBinaryResponseBody{BuildBinaryRuntimeErrorResponse<Response>(
              scada::StatusCode::Bad_SessionIsLoggedOff)});
    }

    const auto response =
        co_await runtime_.Handle<Response>(connection_, typed_request);
    co_return EncodeOpcUaBinaryServiceResponse(header.request_handle,
                                               OpcUaBinaryResponseBody{response});
  }

 [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryReadRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryBrowseRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryBrowseNextRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryTranslateBrowsePathsRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryCallRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryHistoryReadRawRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryWriteRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryDeleteNodesRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryAddNodesRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryDeleteReferencesRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryAddReferencesRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryCreateSessionRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryActivateSessionRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryCloseSessionRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryCreateSubscriptionRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryModifySubscriptionRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinarySetPublishingModeRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryDeleteSubscriptionsRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryCreateMonitoredItemsRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryModifyMonitoredItemsRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryPublishRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryRepublishRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryTransferSubscriptionsRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinaryDeleteMonitoredItemsRequest& typed_request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const OpcUaBinarySetMonitoringModeRequest& typed_request);
  template <typename Request>
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleRequest(const OpcUaBinaryDecodedRequest& request,
                const Request& typed_request) {
    (void)request;
    (void)typed_request;
    co_return std::nullopt;
  }

  OpcUaBinaryRuntime& runtime_;
  opcua_ws::OpcUaWsSessionManager& session_manager_;
  OpcUaBinaryConnectionState& connection_;
  OpcUaBinarySessionService session_service_;
};

}  // namespace opcua
