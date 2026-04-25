#pragma once

#include "base/awaitable.h"
#include "base/time/time.h"
#include "opcua/message.h"
#include "opcua/service_handler.h"
#include "opcua/server_session.h"
#include "opcua/server_session_manager.h"

#include <optional>
#include <unordered_map>

namespace opcua {

struct ConnectionState {
  std::optional<scada::NodeId> authentication_token;
  bool closed = false;
};

struct ServerRuntimeContext {
  AnyExecutor executor;
  ServerSessionManager& session_manager;
  scada::MonitoredItemService& monitored_item_service;
  scada::AttributeService& attribute_service;
  scada::ViewService& view_service;
  scada::HistoryService& history_service;
  scada::MethodService& method_service;
  scada::NodeManagementService& node_management_service;
  std::function<base::Time()> now = &base::Time::Now;
  // Optional override for delayed task scheduling. Defaults to
  // boost::asio::steady_timer-based posting when null.
  std::function<void(base::TimeDelta, std::function<void()>)> post_delayed_task;
};

class ServerRuntime : private ServerRuntimeContext {
 public:
  explicit ServerRuntime(ServerRuntimeContext&& context);

  [[nodiscard]] Awaitable<ResponseBody> Handle(
      ConnectionState& connection,
      RequestBody request);
  void Detach(ConnectionState& connection);

 private:
  using SessionMap =
      std::unordered_map<scada::NodeId, std::shared_ptr<ServerSession>>;

  [[nodiscard]] ServerSession* FindSession(
      const scada::NodeId& authentication_token) const;
  [[nodiscard]] ServerSession* FindAttachedSession(
      const ConnectionState& connection) const;
  void ForgetSession(const scada::NodeId& authentication_token);
  void IndexSessionSubscriptions(
      const scada::NodeId& authentication_token,
      const ServerSession& session);
  void RemoveSessionSubscriptions(const scada::NodeId& authentication_token);
  [[nodiscard]] Awaitable<ResponseBody> HandleActivateSession(
      ConnectionState& connection,
      ActivateSessionRequest request);
  [[nodiscard]] Awaitable<void> Delay(base::TimeDelta delay) const;

  SessionMap sessions_;
  std::unordered_map<SubscriptionId, scada::NodeId> subscription_owners_;
  SubscriptionId next_subscription_id_ = 1;
};

}  // namespace opcua
