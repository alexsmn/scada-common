#include "opcua/service_handler.h"

#include "opcua/endpoint_core.h"

#include "scada/coroutine_services.h"

#include <type_traits>
#include <utility>

namespace opcua {

ServiceHandler::ServiceHandler(ServiceHandlerContext&& context)
    : ServiceHandlerContext{std::move(context)},
      attribute_service_adapter_{
          std::make_unique<scada::CallbackToCoroutineAttributeServiceAdapter>(
              executor, attribute_service)},
      view_service_adapter_{
          std::make_unique<scada::CallbackToCoroutineViewServiceAdapter>(
              executor, view_service)},
      history_service_adapter_{
          std::make_unique<scada::CallbackToCoroutineHistoryServiceAdapter>(
              executor, history_service)},
      method_service_adapter_{
          std::make_unique<scada::CallbackToCoroutineMethodServiceAdapter>(
              executor, method_service)},
      node_management_service_adapter_{
          std::make_unique<
              scada::CallbackToCoroutineNodeManagementServiceAdapter>(
              executor, node_management_service)} {}

ServiceHandler::~ServiceHandler() = default;

Awaitable<ServiceResponse> ServiceHandler::Handle(
    ServiceRequest request) const {
  auto typed_response = co_await std::visit(
      [this](auto&& typed_request) -> Awaitable<ServiceResponse> {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, ReadRequest>) {
          co_return co_await HandleRead(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, WriteRequest>) {
          co_return co_await HandleWrite(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, BrowseRequest>) {
          co_return co_await HandleBrowse(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, BrowseNextRequest>) {
          co_return ServiceResponse{
              BrowseNextResponse{.status = scada::StatusCode::Bad}};
        } else if constexpr (std::is_same_v<T, TranslateBrowsePathsRequest>) {
          co_return co_await HandleTranslateBrowsePaths(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, CallRequest>) {
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

Awaitable<ServiceResponse> ServiceHandler::HandleRead(
    ReadRequest request) const {
  auto [status, results] = co_await attribute_service_adapter_->Read(
      MakeServiceContext(user_id),
      std::make_shared<const std::vector<scada::ReadValueId>>(
          std::move(request.inputs)));
  results = NormalizeReadResults(std::move(results));
  co_return ServiceResponse{
      ReadResponse{std::move(status), std::move(results)}};
}

Awaitable<ServiceResponse> ServiceHandler::HandleWrite(
    WriteRequest request) const {
  auto [status, results] = co_await attribute_service_adapter_->Write(
      MakeServiceContext(user_id),
      std::make_shared<const std::vector<scada::WriteValue>>(
          std::move(request.inputs)));
  co_return ServiceResponse{
      WriteResponse{std::move(status), std::move(results)}};
}

Awaitable<ServiceResponse> ServiceHandler::HandleBrowse(
    BrowseRequest request) const {
  auto [status, results] = co_await view_service_adapter_->Browse(
      MakeServiceContext(user_id), std::move(request.inputs));
  co_return ServiceResponse{
      BrowseResponse{std::move(status), std::move(results)}};
}

Awaitable<ServiceResponse>
ServiceHandler::HandleTranslateBrowsePaths(
    TranslateBrowsePathsRequest request) const {
  auto [status, results] = co_await view_service_adapter_->TranslateBrowsePaths(
      std::move(request.inputs));
  co_return ServiceResponse{
      TranslateBrowsePathsResponse{std::move(status), std::move(results)}};
}

Awaitable<ServiceResponse> ServiceHandler::HandleCall(
    CallRequest request) const {
  CallResponse response;
  response.results.reserve(request.methods.size());
  for (auto& method : request.methods) {
    auto status = co_await method_service_adapter_->Call(
        std::move(method.object_id), std::move(method.method_id),
        std::move(method.arguments), user_id);
    response.results.push_back(MethodCallResult{std::move(status)});
  }

  co_return ServiceResponse{std::move(response)};
}

Awaitable<ServiceResponse> ServiceHandler::HandleHistoryReadRaw(
    HistoryReadRawRequest request) const {
  auto result = co_await history_service_adapter_->HistoryReadRaw(
      std::move(request.details));
  co_return ServiceResponse{HistoryReadRawResponse{std::move(result)}};
}

Awaitable<ServiceResponse> ServiceHandler::HandleHistoryReadEvents(
    HistoryReadEventsRequest request) const {
  auto result = co_await history_service_adapter_->HistoryReadEvents(
      std::move(request.details.node_id), request.details.from,
      request.details.to, std::move(request.details.filter));
  co_return ServiceResponse{
      HistoryReadEventsResponse{std::move(result)}};
}

Awaitable<ServiceResponse> ServiceHandler::HandleAddNodes(
    AddNodesRequest request) const {
  auto [status, results] = co_await node_management_service_adapter_->AddNodes(
      std::move(request.items));
  co_return ServiceResponse{
      AddNodesResponse{std::move(status), std::move(results)}};
}

Awaitable<ServiceResponse> ServiceHandler::HandleDeleteNodes(
    DeleteNodesRequest request) const {
  auto [status, results] =
      co_await node_management_service_adapter_->DeleteNodes(
          std::move(request.items));
  co_return ServiceResponse{
      DeleteNodesResponse{std::move(status), std::move(results)}};
}

Awaitable<ServiceResponse> ServiceHandler::HandleAddReferences(
    AddReferencesRequest request) const {
  auto [status, results] =
      co_await node_management_service_adapter_->AddReferences(
          std::move(request.items));
  co_return ServiceResponse{
      AddReferencesResponse{std::move(status), std::move(results)}};
}

Awaitable<ServiceResponse> ServiceHandler::HandleDeleteReferences(
    DeleteReferencesRequest request) const {
  auto [status, results] =
      co_await node_management_service_adapter_->DeleteReferences(
          std::move(request.items));
  co_return ServiceResponse{
      DeleteReferencesResponse{std::move(status), std::move(results)}};
}

}  // namespace opcua
