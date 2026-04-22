#include "opcua/opcua_binary_service_dispatcher.h"
#include "opcua/opcua_binary_service_codec.h"

namespace opcua {
namespace {
std::optional<std::vector<char>> EncodeBinaryResponse(
    scada::UInt32 request_handle,
    const OpcUaBinaryResponseBody& response) {
  return EncodeOpcUaBinaryServiceResponse(request_handle, response);
}

std::optional<std::vector<char>> EncodeBinaryResponse(
    const OpcUaBinaryDecodedRequest& request) {
  return std::visit(
      [&request](const auto& typed_response) -> std::optional<std::vector<char>> {
        using T = std::decay_t<decltype(typed_response)>;
        if constexpr (std::is_same_v<T, OpcUaBinaryHistoryReadEventsResponse>) {
          return EncodeOpcUaBinaryHistoryReadEventsResponse(
              request.header.request_handle, typed_response,
              request.history_event_field_paths);
        } else {
          return EncodeBinaryResponse(request.header.request_handle,
                                      OpcUaBinaryResponseBody{typed_response});
        }
      },
      request.body);
}
}  // namespace

OpcUaBinaryServiceDispatcher::OpcUaBinaryServiceDispatcher(Context context)
    : runtime_{context.runtime},
      connection_{context.connection} {}

Awaitable<std::optional<std::vector<char>>> OpcUaBinaryServiceDispatcher::HandlePayload(
    std::vector<char> payload) {
  const auto request = DecodeOpcUaBinaryServiceRequest(payload);
  if (!request.has_value()) {
    co_return std::nullopt;
  }

  auto response = co_await runtime_.HandleDecodedRequest(connection_, *request);
  if (!response.has_value()) {
    co_return std::nullopt;
  }

  co_return EncodeBinaryResponse(
      OpcUaBinaryDecodedRequest{
          .header = request->header,
          .body = std::move(*response),
          .history_event_field_paths = request->history_event_field_paths,
      });
}

}  // namespace opcua
