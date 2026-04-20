#pragma once

#include "base/awaitable.h"
#include "base/executor.h"
#include "opcua_ws/opcua_ws_message.h"
#include "opcua_ws/opcua_ws_service_handler.h"
#include "opcua_ws/opcua_ws_session.h"
#include "opcua_ws/opcua_ws_session_manager.h"

#include <optional>
#include <unordered_map>

namespace opcua_ws {

struct OpcUaWsConnectionState {
  std::optional<scada::NodeId> authentication_token;
};

struct OpcUaWsRuntimeContext {
  std::shared_ptr<Executor> executor;
  OpcUaWsSessionManager& session_manager;
  scada::MonitoredItemService& monitored_item_service;
  scada::AttributeService& attribute_service;
  scada::ViewService& view_service;
  scada::HistoryService& history_service;
  scada::MethodService& method_service;
  scada::NodeManagementService& node_management_service;
};

class OpcUaWsRuntime : private OpcUaWsRuntimeContext {
 public:
  explicit OpcUaWsRuntime(OpcUaWsRuntimeContext&& context);

  [[nodiscard]] Awaitable<OpcUaWsResponseMessage> Handle(
      OpcUaWsConnectionState& connection,
      OpcUaWsRequestMessage request);
  void Detach(OpcUaWsConnectionState& connection);

 private:
  using SessionMap = std::unordered_map<scada::NodeId, std::shared_ptr<OpcUaWsSession>>;

  [[nodiscard]] OpcUaWsSession* FindSession(
      const scada::NodeId& authentication_token) const;
  [[nodiscard]] OpcUaWsSession* FindAttachedSession(
      const OpcUaWsConnectionState& connection) const;
  void ForgetSession(const scada::NodeId& authentication_token);
  void IndexSessionSubscriptions(
      const scada::NodeId& authentication_token,
      const OpcUaWsSession& session);
  void RemoveSessionSubscriptions(const scada::NodeId& authentication_token);

  [[nodiscard]] Awaitable<OpcUaWsResponseBody> HandleRequestBody(
      OpcUaWsConnectionState& connection,
      OpcUaWsRequestBody request);
  [[nodiscard]] Awaitable<OpcUaWsResponseBody> HandleActivateSession(
      OpcUaWsConnectionState& connection,
      OpcUaWsActivateSessionRequest request);

  SessionMap sessions_;
  std::unordered_map<OpcUaWsSubscriptionId, scada::NodeId> subscription_owners_;
  OpcUaWsSubscriptionId next_subscription_id_ = 1;
};

}  // namespace opcua_ws
