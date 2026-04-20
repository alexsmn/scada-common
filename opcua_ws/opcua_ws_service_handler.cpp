#include "opcua_ws/opcua_ws_service_handler.h"

#include "scada/service_awaitable.h"

#include <type_traits>
#include <utility>

namespace opcua_ws {

OpcUaWsServiceHandler::OpcUaWsServiceHandler(
    OpcUaWsServiceHandlerContext&& context)
    : OpcUaWsServiceHandlerContext{std::move(context)} {}

Awaitable<OpcUaWsServiceResponse> OpcUaWsServiceHandler::Handle(
    OpcUaWsServiceRequest request) const {
  auto typed_response = co_await std::visit(
      [this](auto&& typed_request) -> Awaitable<OpcUaWsServiceResponse> {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, CallRequest>) {
          co_return co_await HandleCall(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, HistoryReadRawRequest>) {
          co_return co_await HandleHistoryReadRaw(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, HistoryReadEventsRequest>) {
          co_return co_await HandleHistoryReadEvents(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, AddNodesRequest>) {
          co_return co_await HandleAddNodes(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, DeleteNodesRequest>) {
          co_return co_await HandleDeleteNodes(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, AddReferencesRequest>) {
          co_return co_await HandleAddReferences(std::move(typed_request));
        } else {
          co_return co_await HandleDeleteReferences(std::move(typed_request));
        }
      },
      std::move(request));
  co_return typed_response;
}

Awaitable<OpcUaWsServiceResponse> OpcUaWsServiceHandler::HandleCall(
    CallRequest request) const {
  CallResponse response;
  response.results.reserve(request.methods.size());
  for (auto& method : request.methods) {
    auto status =
        co_await scada::CallAsync(executor, method_service,
                                  std::move(method.object_id),
                                  std::move(method.method_id),
                                  std::move(method.arguments), user_id);
    response.results.push_back(CallMethodResult{std::move(status)});
  }

  co_return OpcUaWsServiceResponse{std::move(response)};
}

Awaitable<OpcUaWsServiceResponse> OpcUaWsServiceHandler::HandleHistoryReadRaw(
    HistoryReadRawRequest request) const {
  auto result = co_await scada::HistoryReadRawAsync(executor, history_service,
                                                    std::move(request.details));
  co_return OpcUaWsServiceResponse{HistoryReadRawResponse{std::move(result)}};
}

Awaitable<OpcUaWsServiceResponse>
OpcUaWsServiceHandler::HandleHistoryReadEvents(
    HistoryReadEventsRequest request) const {
  auto result = co_await scada::HistoryReadEventsAsync(
      executor, history_service, std::move(request.details.node_id),
      request.details.from, request.details.to, std::move(request.details.filter));
  co_return OpcUaWsServiceResponse{
      HistoryReadEventsResponse{std::move(result)}};
}

Awaitable<OpcUaWsServiceResponse> OpcUaWsServiceHandler::HandleAddNodes(
    AddNodesRequest request) const {
  auto [status, results] = co_await scada::AddNodesAsync(
      executor, node_management_service, std::move(request.items));
  co_return OpcUaWsServiceResponse{
      AddNodesResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaWsServiceResponse> OpcUaWsServiceHandler::HandleDeleteNodes(
    DeleteNodesRequest request) const {
  auto [status, results] = co_await scada::DeleteNodesAsync(
      executor, node_management_service, std::move(request.items));
  co_return OpcUaWsServiceResponse{
      DeleteNodesResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaWsServiceResponse> OpcUaWsServiceHandler::HandleAddReferences(
    AddReferencesRequest request) const {
  auto [status, results] = co_await scada::AddReferencesAsync(
      executor, node_management_service, std::move(request.items));
  co_return OpcUaWsServiceResponse{
      AddReferencesResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaWsServiceResponse> OpcUaWsServiceHandler::HandleDeleteReferences(
    DeleteReferencesRequest request) const {
  auto [status, results] = co_await scada::DeleteReferencesAsync(
      executor, node_management_service, std::move(request.items));
  co_return OpcUaWsServiceResponse{
      DeleteReferencesResponse{std::move(status), std::move(results)}};
}

}  // namespace opcua_ws
