#pragma once

#include "base/awaitable.h"
#include "opcua/service_message.h"

#include "scada/attribute_service.h"
#include "scada/history_service.h"
#include "scada/method_service.h"
#include "scada/node_management_service.h"
#include "scada/view_service.h"

#include <memory>

namespace scada {
class CallbackToCoroutineAttributeServiceAdapter;
class CallbackToCoroutineHistoryServiceAdapter;
class CallbackToCoroutineMethodServiceAdapter;
class CallbackToCoroutineNodeManagementServiceAdapter;
class CallbackToCoroutineViewServiceAdapter;
}  // namespace scada

namespace opcua {

struct ServiceHandlerContext {
  AnyExecutor executor;
  scada::AttributeService& attribute_service;
  scada::ViewService& view_service;
  scada::HistoryService& history_service;
  scada::MethodService& method_service;
  scada::NodeManagementService& node_management_service;
  scada::NodeId user_id;
};

class ServiceHandler : private ServiceHandlerContext {
 public:
  explicit ServiceHandler(ServiceHandlerContext&& context);
  ~ServiceHandler();

  [[nodiscard]] Awaitable<ServiceResponse> Handle(
      ServiceRequest request) const;

 private:
  [[nodiscard]] Awaitable<ServiceResponse> HandleRead(
      ReadRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleWrite(
      WriteRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleBrowse(
      BrowseRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleTranslateBrowsePaths(
      TranslateBrowsePathsRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleCall(
      CallRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleHistoryReadRaw(
      HistoryReadRawRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleHistoryReadEvents(
      HistoryReadEventsRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleAddNodes(
      AddNodesRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleDeleteNodes(
      DeleteNodesRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleAddReferences(
      AddReferencesRequest request) const;
  [[nodiscard]] Awaitable<ServiceResponse> HandleDeleteReferences(
      DeleteReferencesRequest request) const;

  std::unique_ptr<scada::CallbackToCoroutineAttributeServiceAdapter>
      attribute_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineViewServiceAdapter>
      view_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineHistoryServiceAdapter>
      history_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineMethodServiceAdapter>
      method_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineNodeManagementServiceAdapter>
      node_management_service_adapter_;
};

}  // namespace opcua
