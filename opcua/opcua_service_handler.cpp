#include "opcua/opcua_service_handler.h"

#include "opcua/opcua_endpoint_core.h"

#include "base/callback_awaitable.h"
#include "scada/service_awaitable.h"

#include <type_traits>
#include <utility>

namespace opcua {
namespace {

Awaitable<std::tuple<scada::Status, std::vector<scada::DataValue>>>
EndpointReadAsync(
    const std::shared_ptr<Executor>& executor,
    scada::AttributeService& attribute_service,
    const scada::NodeId& user_id,
    std::vector<scada::ReadValueId> inputs) {
  co_return co_await
      CallbackToAwaitable<scada::Status, std::vector<scada::DataValue>>(
          executor,
          [&attribute_service, user_id,
           inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
               std::move(inputs))](auto callback) mutable {
            scada::opcua_endpoint::Read(
                attribute_service,
                scada::opcua_endpoint::MakeServiceContext(user_id),
                std::move(inputs), std::move(callback));
          });
}

Awaitable<std::tuple<scada::Status, std::vector<scada::StatusCode>>>
EndpointWriteAsync(
    const std::shared_ptr<Executor>& executor,
    scada::AttributeService& attribute_service,
    const scada::NodeId& user_id,
    std::vector<scada::WriteValue> inputs) {
  co_return co_await
      CallbackToAwaitable<scada::Status, std::vector<scada::StatusCode>>(
          executor,
          [&attribute_service, user_id,
           inputs = std::make_shared<const std::vector<scada::WriteValue>>(
               std::move(inputs))](auto callback) mutable {
            scada::opcua_endpoint::Write(
                attribute_service,
                scada::opcua_endpoint::MakeServiceContext(user_id),
                std::move(inputs), std::move(callback));
          });
}

Awaitable<std::tuple<scada::Status, std::vector<scada::BrowseResult>>>
EndpointBrowseAsync(const std::shared_ptr<Executor>& executor,
                    scada::ViewService& view_service,
                    const scada::NodeId& user_id,
                    std::vector<scada::BrowseDescription> inputs) {
  co_return co_await
      CallbackToAwaitable<scada::Status, std::vector<scada::BrowseResult>>(
          executor,
          [&view_service, user_id,
           inputs = std::move(inputs)](auto callback) mutable {
            scada::opcua_endpoint::Browse(
                view_service, scada::opcua_endpoint::MakeServiceContext(user_id),
                std::move(inputs), std::move(callback));
          });
}

Awaitable<std::tuple<scada::Status, std::vector<scada::BrowsePathResult>>>
EndpointTranslateBrowsePathsAwaitable(
    const std::shared_ptr<Executor>& executor,
    scada::ViewService& view_service,
    std::vector<scada::BrowsePath> inputs) {
  co_return co_await
      CallbackToAwaitable<scada::Status, std::vector<scada::BrowsePathResult>>(
          executor,
          [&view_service, inputs = std::move(inputs)](auto callback) mutable {
            scada::opcua_endpoint::TranslateBrowsePaths(
                view_service, std::move(inputs), std::move(callback));
          });
}

Awaitable<scada::Status> EndpointCallAsync(
    const std::shared_ptr<Executor>& executor,
    scada::MethodService& method_service,
    scada::NodeId node_id,
    scada::NodeId method_id,
    std::vector<scada::Variant> arguments,
    scada::NodeId user_id) {
  auto [status] = co_await CallbackToAwaitable<scada::Status>(
      executor, [&method_service, node_id = std::move(node_id),
                 method_id = std::move(method_id),
                 arguments = std::move(arguments),
                 user_id = std::move(user_id)](auto callback) mutable {
        scada::opcua_endpoint::CallMethod(method_service, node_id, method_id,
                                          arguments, user_id,
                                          std::move(callback));
      });
  co_return std::move(status);
}

Awaitable<std::tuple<scada::Status, std::vector<scada::AddNodesResult>>>
EndpointAddNodesAsync(const std::shared_ptr<Executor>& executor,
                      scada::NodeManagementService& node_management_service,
                      std::vector<scada::AddNodesItem> inputs) {
  co_return co_await
      CallbackToAwaitable<scada::Status, std::vector<scada::AddNodesResult>>(
          executor,
          [&node_management_service,
           inputs = std::move(inputs)](auto callback) mutable {
            scada::opcua_endpoint::AddNodes(
                node_management_service, std::move(inputs),
                std::move(callback));
          });
}

Awaitable<std::tuple<scada::Status, std::vector<scada::StatusCode>>>
EndpointDeleteNodesAsync(const std::shared_ptr<Executor>& executor,
                         scada::NodeManagementService& node_management_service,
                         std::vector<scada::DeleteNodesItem> inputs) {
  co_return co_await
      CallbackToAwaitable<scada::Status, std::vector<scada::StatusCode>>(
          executor,
          [&node_management_service,
           inputs = std::move(inputs)](auto callback) mutable {
            scada::opcua_endpoint::DeleteNodes(
                node_management_service, std::move(inputs),
                std::move(callback));
          });
}

}  // namespace

OpcUaServiceHandler::OpcUaServiceHandler(OpcUaServiceHandlerContext&& context)
    : OpcUaServiceHandlerContext{std::move(context)} {}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::Handle(
    OpcUaServiceRequest request) const {
  auto typed_response = co_await std::visit(
      [this](auto&& typed_request) -> Awaitable<OpcUaServiceResponse> {
        using T = std::decay_t<decltype(typed_request)>;
        if constexpr (std::is_same_v<T, ReadRequest>) {
          co_return co_await HandleRead(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, WriteRequest>) {
          co_return co_await HandleWrite(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, BrowseRequest>) {
          co_return co_await HandleBrowse(std::move(typed_request));
        } else if constexpr (std::is_same_v<T, BrowseNextRequest>) {
          co_return OpcUaServiceResponse{
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

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleRead(
    ReadRequest request) const {
  auto [status, results] =
      co_await EndpointReadAsync(executor, attribute_service, user_id,
                                 std::move(request.inputs));
  co_return OpcUaServiceResponse{
      ReadResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleWrite(
    WriteRequest request) const {
  auto [status, results] =
      co_await EndpointWriteAsync(executor, attribute_service, user_id,
                                  std::move(request.inputs));
  co_return OpcUaServiceResponse{
      WriteResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleBrowse(
    BrowseRequest request) const {
  auto [status, results] =
      co_await EndpointBrowseAsync(executor, view_service, user_id,
                                   std::move(request.inputs));
  co_return OpcUaServiceResponse{
      BrowseResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaServiceResponse>
OpcUaServiceHandler::HandleTranslateBrowsePaths(
    TranslateBrowsePathsRequest request) const {
  auto [status, results] = co_await EndpointTranslateBrowsePathsAwaitable(
      executor, view_service, std::move(request.inputs));
  co_return OpcUaServiceResponse{
      TranslateBrowsePathsResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleCall(
    CallRequest request) const {
  CallResponse response;
  response.results.reserve(request.methods.size());
  for (auto& method : request.methods) {
    auto status = co_await EndpointCallAsync(
        executor, method_service, std::move(method.object_id),
        std::move(method.method_id), std::move(method.arguments), user_id);
    response.results.push_back(OpcUaMethodCallResult{std::move(status)});
  }

  co_return OpcUaServiceResponse{std::move(response)};
}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleHistoryReadRaw(
    HistoryReadRawRequest request) const {
  auto result = co_await scada::HistoryReadRawAsync(executor, history_service,
                                                    std::move(request.details));
  co_return OpcUaServiceResponse{HistoryReadRawResponse{std::move(result)}};
}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleHistoryReadEvents(
    HistoryReadEventsRequest request) const {
  auto result = co_await scada::HistoryReadEventsAsync(
      executor, history_service, std::move(request.details.node_id),
      request.details.from, request.details.to, std::move(request.details.filter));
  co_return OpcUaServiceResponse{
      HistoryReadEventsResponse{std::move(result)}};
}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleAddNodes(
    AddNodesRequest request) const {
  auto [status, results] = co_await EndpointAddNodesAsync(
      executor, node_management_service, std::move(request.items));
  co_return OpcUaServiceResponse{
      AddNodesResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleDeleteNodes(
    DeleteNodesRequest request) const {
  auto [status, results] = co_await EndpointDeleteNodesAsync(
      executor, node_management_service, std::move(request.items));
  co_return OpcUaServiceResponse{
      DeleteNodesResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleAddReferences(
    AddReferencesRequest request) const {
  auto [status, results] = co_await scada::AddReferencesAsync(
      executor, node_management_service, std::move(request.items));
  co_return OpcUaServiceResponse{
      AddReferencesResponse{std::move(status), std::move(results)}};
}

Awaitable<OpcUaServiceResponse> OpcUaServiceHandler::HandleDeleteReferences(
    DeleteReferencesRequest request) const {
  auto [status, results] = co_await scada::DeleteReferencesAsync(
      executor, node_management_service, std::move(request.items));
  co_return OpcUaServiceResponse{
      DeleteReferencesResponse{std::move(status), std::move(results)}};
}

}  // namespace opcua
