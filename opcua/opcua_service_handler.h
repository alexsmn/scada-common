#pragma once

#include "base/awaitable.h"
#include "base/executor.h"
#include "opcua/opcua_service_message.h"

#include "scada/attribute_service.h"
#include "scada/history_service.h"
#include "scada/method_service.h"
#include "scada/node_management_service.h"
#include "scada/view_service.h"

#include <memory>

namespace opcua {

struct OpcUaServiceHandlerContext {
  std::shared_ptr<Executor> executor;
  scada::AttributeService& attribute_service;
  scada::ViewService& view_service;
  scada::HistoryService& history_service;
  scada::MethodService& method_service;
  scada::NodeManagementService& node_management_service;
  scada::NodeId user_id;
};

class OpcUaServiceHandler : private OpcUaServiceHandlerContext {
 public:
  explicit OpcUaServiceHandler(OpcUaServiceHandlerContext&& context);

  [[nodiscard]] Awaitable<OpcUaServiceResponse> Handle(
      OpcUaServiceRequest request) const;

 private:
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleRead(
      ReadRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleWrite(
      WriteRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleBrowse(
      BrowseRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleTranslateBrowsePaths(
      TranslateBrowsePathsRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleCall(
      CallRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleHistoryReadRaw(
      HistoryReadRawRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleHistoryReadEvents(
      HistoryReadEventsRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleAddNodes(
      AddNodesRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleDeleteNodes(
      DeleteNodesRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleAddReferences(
      AddReferencesRequest request) const;
  [[nodiscard]] Awaitable<OpcUaServiceResponse> HandleDeleteReferences(
      DeleteReferencesRequest request) const;
};

}  // namespace opcua
