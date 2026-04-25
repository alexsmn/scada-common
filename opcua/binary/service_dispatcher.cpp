#include "opcua/binary/service_dispatcher.h"
#include "opcua/binary/service_codec.h"

namespace opcua::binary {
namespace {
std::optional<std::vector<char>> EncodeResponse(
    scada::UInt32 request_handle,
    const ResponseBody& response) {
  return EncodeServiceResponse(request_handle, response);
}

std::optional<std::vector<char>> EncodeResponse(
    scada::UInt32 request_handle,
    const std::vector<std::vector<std::string>>& history_event_field_paths,
    const ResponseBody& response) {
  return std::visit(
      [request_handle, &history_event_field_paths](
          const auto& typed_response) -> std::optional<std::vector<char>> {
        using T = std::decay_t<decltype(typed_response)>;
        if constexpr (std::is_same_v<T, HistoryReadEventsResponse>) {
          return EncodeHistoryReadEventsResponse(
              request_handle, typed_response, history_event_field_paths);
        } else {
          return EncodeResponse(request_handle,
                                      ResponseBody{typed_response});
        }
      },
      response);
}
}  // namespace

ServiceDispatcher::ServiceDispatcher(Context context)
    : runtime_{context.runtime},
      connection_{context.connection} {}

Awaitable<std::optional<std::vector<char>>> ServiceDispatcher::HandlePayload(
    std::vector<char> payload) {
  const auto request = DecodeServiceRequest(payload);
  if (!request.has_value()) {
    co_return std::nullopt;
  }

  auto response = co_await runtime_.HandleDecodedRequest(connection_, *request);
  if (!response.has_value()) {
    co_return std::nullopt;
  }

  co_return EncodeResponse(request->header.request_handle,
                                 request->history_event_field_paths,
                                 *response);
}

}  // namespace opcua::binary
