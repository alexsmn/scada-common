#pragma once

#include "base/awaitable.h"
#include "opcua/opcua_binary_message.h"
#include "opcua/opcua_binary_service_codec.h"
#include "opcua/opcua_runtime.h"

namespace opcua {

template <typename Response>
Response BuildBinaryRuntimeErrorResponse(scada::Status status) {
  if constexpr (requires(Response response) { response.status; }) {
    return Response{.status = std::move(status)};
  } else if constexpr (requires(Response response) { response.result.status; }) {
    auto response = Response{};
    response.result.status = std::move(status);
    return response;
  } else if constexpr (requires(Response response) { response.results; }) {
    using Result = typename decltype(Response{}.results)::value_type;
    if constexpr (requires(Result result) { result.status; }) {
      return Response{.results = {Result{.status = std::move(status)}}};
    } else {
      return Response{};
    }
  } else {
    return Response{};
  }
}

using OpcUaBinaryConnectionState = OpcUaConnectionState;

struct OpcUaBinaryRuntimeContext {
  std::shared_ptr<Executor> executor;
  OpcUaSessionManager& session_manager;
  scada::MonitoredItemService& monitored_item_service;
  scada::AttributeService& attribute_service;
  scada::ViewService& view_service;
  scada::HistoryService& history_service;
  scada::MethodService& method_service;
  scada::NodeManagementService& node_management_service;
  std::function<base::Time()> now = &base::Time::Now;
};

// UA Binary reuses the canonical shared server-side session/subscription/
// service runtime while keeping Binary-specific framing and codec logic local.
class OpcUaBinaryRuntime {
 public:
  explicit OpcUaBinaryRuntime(OpcUaBinaryRuntimeContext&& context);

  template <typename Response, typename Request>
  [[nodiscard]] Awaitable<Response> Handle(OpcUaBinaryConnectionState& connection,
                                           Request request) {
    auto body = co_await HandleBody(connection, OpcUaBinaryRequestBody{
                                                    std::move(request)});
    if (auto* typed = std::get_if<Response>(&body)) {
      co_return std::move(*typed);
    }
    if (auto* fault = std::get_if<OpcUaBinaryServiceFault>(&body)) {
      co_return BuildBinaryRuntimeErrorResponse<Response>(fault->status);
    }
    co_return BuildBinaryRuntimeErrorResponse<Response>(scada::StatusCode::Bad);
  }

  void Detach(OpcUaBinaryConnectionState& connection);

  [[nodiscard]] Awaitable<std::optional<OpcUaBinaryResponseBody>>
  HandleDecodedRequest(OpcUaBinaryConnectionState& connection,
                       const OpcUaBinaryDecodedRequest& request);

 private:
  [[nodiscard]] Awaitable<OpcUaResponseBody> HandleBody(
      OpcUaBinaryConnectionState& connection,
      OpcUaRequestBody request);

  template <typename Response, typename Request>
  [[nodiscard]] Awaitable<std::optional<OpcUaBinaryResponseBody>>
  HandleAuthenticatedRequest(OpcUaBinaryConnectionState& connection,
                             const OpcUaBinaryDecodedRequest& request,
                             Request typed_request) {
    if (!connection.authentication_token.has_value() ||
        *connection.authentication_token != request.header.authentication_token) {
      co_return OpcUaBinaryResponseBody{BuildBinaryRuntimeErrorResponse<Response>(
          scada::StatusCode::Bad_SessionIsLoggedOff)};
    }

    co_return OpcUaBinaryResponseBody{
        co_await Handle<Response>(connection, std::move(typed_request))};
  }

  [[nodiscard]] Awaitable<std::optional<OpcUaBinaryResponseBody>>
  HandleSessionRequest(OpcUaBinaryConnectionState& connection,
                       OpcUaCreateSessionRequest request);
  [[nodiscard]] Awaitable<std::optional<OpcUaBinaryResponseBody>>
  HandleSessionRequest(OpcUaBinaryConnectionState& connection,
                       const OpcUaBinaryServiceRequestHeader& header,
                       OpcUaActivateSessionRequest request);
  [[nodiscard]] Awaitable<std::optional<OpcUaBinaryResponseBody>>
  HandleSessionRequest(OpcUaBinaryConnectionState& connection,
                       const OpcUaBinaryServiceRequestHeader& header,
                       OpcUaCloseSessionRequest request);

  OpcUaSessionManager& session_manager_;
  OpcUaRuntime runtime_;
};

}  // namespace opcua
