#pragma once

#include "base/awaitable.h"
#include "base/executor.h"
#include "opcua_ws/opcua_ws_service_message.h"

#include "scada/history_service.h"
#include "scada/method_service.h"
#include "scada/node_management_service.h"

#include <memory>

namespace opcua_ws {

struct OpcUaWsServiceHandlerContext {
  std::shared_ptr<Executor> executor;
  scada::HistoryService& history_service;
  scada::MethodService& method_service;
  scada::NodeManagementService& node_management_service;
  scada::NodeId user_id;
};

class OpcUaWsServiceHandler : private OpcUaWsServiceHandlerContext {
 public:
  explicit OpcUaWsServiceHandler(OpcUaWsServiceHandlerContext&& context);

  [[nodiscard]] Awaitable<OpcUaWsServiceResponse> Handle(
      OpcUaWsServiceRequest request) const;

 private:
  [[nodiscard]] Awaitable<OpcUaWsServiceResponse> HandleCall(
      CallRequest request) const;
  [[nodiscard]] Awaitable<OpcUaWsServiceResponse> HandleHistoryReadRaw(
      HistoryReadRawRequest request) const;
  [[nodiscard]] Awaitable<OpcUaWsServiceResponse> HandleHistoryReadEvents(
      HistoryReadEventsRequest request) const;
  [[nodiscard]] Awaitable<OpcUaWsServiceResponse> HandleAddNodes(
      AddNodesRequest request) const;
  [[nodiscard]] Awaitable<OpcUaWsServiceResponse> HandleDeleteNodes(
      DeleteNodesRequest request) const;
  [[nodiscard]] Awaitable<OpcUaWsServiceResponse> HandleAddReferences(
      AddReferencesRequest request) const;
  [[nodiscard]] Awaitable<OpcUaWsServiceResponse> HandleDeleteReferences(
      DeleteReferencesRequest request) const;
};

}  // namespace opcua_ws
