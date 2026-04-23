#include "opcua/binary/opcua_binary_service_dispatcher.h"
#include "opcua/binary/opcua_binary_service_codec.h"

namespace opcua {
namespace {
std::optional<std::vector<char>> EncodeBinaryResponse(
    scada::UInt32 request_handle,
    const OpcUaResponseBody& response) {
  return EncodeOpcUaBinaryServiceResponse(request_handle, response);
}

std::optional<std::vector<char>> EncodeBinaryResponse(
    scada::UInt32 request_handle,
    const std::vector<std::vector<std::string>>& history_event_field_paths,
    const OpcUaResponseBody& response) {
  return std::visit(
      [request_handle, &history_event_field_paths](
          const auto& typed_response) -> std::optional<std::vector<char>> {
        using T = std::decay_t<decltype(typed_response)>;
        if constexpr (std::is_same_v<T, HistoryReadEventsResponse>) {
          return EncodeOpcUaBinaryHistoryReadEventsResponse(
              request_handle, typed_response, history_event_field_paths);
        } else {
          return EncodeBinaryResponse(request_handle,
                                      OpcUaResponseBody{typed_response});
        }
      },
      response);
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

  co_return EncodeBinaryResponse(request->header.request_handle,
                                 request->history_event_field_paths,
                                 *response);
}

}  // namespace opcua
