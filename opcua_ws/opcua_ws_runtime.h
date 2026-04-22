#pragma once

#include "base/awaitable.h"
#include "base/executor.h"
#include "base/time/time.h"
#include "opcua/opcua_message.h"
#include "opcua/opcua_service_handler.h"
#include "opcua_ws/opcua_ws_session.h"
#include "opcua_ws/opcua_ws_session_manager.h"

#include <optional>
#include <unordered_map>

namespace opcua {

using opcua_ws::OpcUaWsSessionManager;
using opcua_ws::OpcUaWsSubscriptionId;

struct OpcUaConnectionState {
  std::optional<scada::NodeId> authentication_token;
  bool closed = false;
};

struct OpcUaRuntimeContext {
  std::shared_ptr<Executor> executor;
  OpcUaWsSessionManager& session_manager;
  scada::MonitoredItemService& monitored_item_service;
  scada::AttributeService& attribute_service;
  scada::ViewService& view_service;
  scada::HistoryService& history_service;
  scada::MethodService& method_service;
  scada::NodeManagementService& node_management_service;
  std::function<base::Time()> now = &base::Time::Now;
};

class OpcUaRuntime : private OpcUaRuntimeContext {
 public:
  explicit OpcUaRuntime(OpcUaRuntimeContext&& context);

  [[nodiscard]] Awaitable<OpcUaResponseBody> Handle(
      OpcUaConnectionState& connection,
      OpcUaRequestBody request);
  void Detach(OpcUaConnectionState& connection);

 private:
  using SessionMap =
      std::unordered_map<scada::NodeId, std::shared_ptr<OpcUaSession>>;

  [[nodiscard]] OpcUaSession* FindSession(
      const scada::NodeId& authentication_token) const;
  [[nodiscard]] OpcUaSession* FindAttachedSession(
      const OpcUaConnectionState& connection) const;
  void ForgetSession(const scada::NodeId& authentication_token);
  void IndexSessionSubscriptions(
      const scada::NodeId& authentication_token,
      const OpcUaSession& session);
  void RemoveSessionSubscriptions(const scada::NodeId& authentication_token);
  [[nodiscard]] Awaitable<OpcUaResponseBody> HandleActivateSession(
      OpcUaConnectionState& connection,
      OpcUaActivateSessionRequest request);
  [[nodiscard]] Awaitable<void> Delay(base::TimeDelta delay) const;

  SessionMap sessions_;
  std::unordered_map<OpcUaWsSubscriptionId, scada::NodeId> subscription_owners_;
  OpcUaWsSubscriptionId next_subscription_id_ = 1;
};

}  // namespace opcua

namespace opcua_ws {

using OpcUaWsConnectionState = opcua::OpcUaConnectionState;
using OpcUaWsRuntimeContext = opcua::OpcUaRuntimeContext;
using OpcUaWsRuntime = opcua::OpcUaRuntime;

}  // namespace opcua_ws
