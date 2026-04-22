#pragma once

#include "base/awaitable.h"
#include "opcua/opcua_binary_service_codec.h"
#include "opcua/opcua_binary_runtime.h"
#include "opcua/opcua_server_session_manager.h"

namespace opcua {

class OpcUaBinaryServiceDispatcher {
 public:
  struct Context {
    OpcUaBinaryRuntime& runtime;
    OpcUaSessionManager& session_manager;
    OpcUaBinaryConnectionState& connection;
  };

  explicit OpcUaBinaryServiceDispatcher(Context context);

  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandlePayload(
      std::vector<char> payload);

 private:
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandleSessionRequest(
      scada::UInt32 request_handle,
      OpcUaBinaryCreateSessionRequest request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandleSessionRequest(
      const OpcUaBinaryServiceRequestHeader& header,
      OpcUaBinaryActivateSessionRequest request);
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>> HandleSessionRequest(
      const OpcUaBinaryServiceRequestHeader& header,
      OpcUaBinaryCloseSessionRequest request);

  template <typename Response, typename Request, typename Encoder>
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleAuthenticatedRequest(const OpcUaBinaryDecodedRequest& request,
                             Request typed_request,
                             Encoder&& encode_response) {
    if (!connection_.authentication_token.has_value() ||
        *connection_.authentication_token != request.header.authentication_token) {
      co_return encode_response(
          request.header.request_handle,
          BuildBinaryRuntimeErrorResponse<Response>(
              scada::StatusCode::Bad_SessionIsLoggedOff));
    }

    const auto response =
        co_await runtime_.Handle<Response>(connection_, std::move(typed_request));
    co_return encode_response(request.header.request_handle, std::move(response));
  }

  template <typename Response, typename Request>
  [[nodiscard]] Awaitable<std::optional<std::vector<char>>>
  HandleAuthenticatedRequest(const OpcUaBinaryDecodedRequest& request,
                             Request typed_request) {
    auto encode = [](scada::UInt32 request_handle, Response response) {
      return EncodeResponse(request_handle, std::move(response));
    };
    co_return co_await HandleAuthenticatedRequest<Response>(
        request, std::move(typed_request), std::move(encode));
  }

  OpcUaBinaryRuntime& runtime_;
  OpcUaSessionManager& session_manager_;
  OpcUaBinaryConnectionState& connection_;
};

}  // namespace opcua
