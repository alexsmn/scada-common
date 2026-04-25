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
class CoroutineAttributeService;
class CoroutineHistoryService;
class CoroutineMethodService;
class CoroutineNodeManagementService;
class CoroutineViewService;
}  // namespace scada

namespace opcua {

struct ServiceHandlerContext {
  scada::CoroutineAttributeService& attribute_service;
  scada::CoroutineViewService& view_service;
  scada::CoroutineHistoryService& history_service;
  scada::CoroutineMethodService& method_service;
  scada::CoroutineNodeManagementService& node_management_service;
  scada::NodeId user_id;
};

class ServiceHandler : private ServiceHandlerContext {
 public:
  explicit ServiceHandler(ServiceHandlerContext&& context);

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
};

}  // namespace opcua
