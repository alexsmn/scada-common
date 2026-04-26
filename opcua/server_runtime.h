#pragma once

#include "base/awaitable.h"
#include "base/time/time.h"
#include "opcua/message.h"
#include "opcua/service_handler.h"
#include "opcua/server_session.h"
#include "opcua/server_session_manager.h"
#include "scada/data_services.h"

#include <optional>
#include <unordered_map>

namespace scada {
class CallbackToCoroutineAttributeServiceAdapter;
class CallbackToCoroutineHistoryServiceAdapter;
class CallbackToCoroutineMethodServiceAdapter;
class CallbackToCoroutineNodeManagementServiceAdapter;
class CallbackToCoroutineViewServiceAdapter;
class CoroutineAttributeService;
class CoroutineHistoryService;
class CoroutineMethodService;
class CoroutineNodeManagementService;
class CoroutineViewService;
}  // namespace scada

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

struct CoroutineServerRuntimeContext {
  AnyExecutor executor;
  ServerSessionManager& session_manager;
  scada::MonitoredItemService& monitored_item_service;
  scada::CoroutineAttributeService& attribute_service;
  scada::CoroutineViewService& view_service;
  scada::CoroutineHistoryService& history_service;
  scada::CoroutineMethodService& method_service;
  scada::CoroutineNodeManagementService& node_management_service;
  std::function<base::Time()> now = &base::Time::Now;
  // Optional override for delayed task scheduling. Defaults to
  // boost::asio::steady_timer-based posting when null.
  std::function<void(base::TimeDelta, std::function<void()>)> post_delayed_task;
};

struct DataServicesServerRuntimeContext {
  AnyExecutor executor;
  ServerSessionManager& session_manager;
  DataServices data_services;
  std::function<base::Time()> now = &base::Time::Now;
  // Optional override for delayed task scheduling. Defaults to
  // boost::asio::steady_timer-based posting when null.
  std::function<void(base::TimeDelta, std::function<void()>)> post_delayed_task;
};

class ServerRuntime {
 public:
  explicit ServerRuntime(ServerRuntimeContext&& context);
  explicit ServerRuntime(CoroutineServerRuntimeContext&& context);
  explicit ServerRuntime(DataServicesServerRuntimeContext&& context);
  ~ServerRuntime();

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
  [[nodiscard]] Awaitable<ServiceResponse> HandleServiceRequest(
      const ServerSession& session,
      ServiceRequest request) const;
  [[nodiscard]] Awaitable<void> Delay(base::TimeDelta delay) const;

  SessionMap sessions_;
  std::unordered_map<SubscriptionId, scada::NodeId> subscription_owners_;
  SubscriptionId next_subscription_id_ = 1;

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

  DataServices data_services_;
  AnyExecutor executor_;
  ServerSessionManager& session_manager_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::CoroutineAttributeService& attribute_service_;
  scada::CoroutineViewService& view_service_;
  scada::CoroutineHistoryService& history_service_;
  scada::CoroutineMethodService& method_service_;
  scada::CoroutineNodeManagementService& node_management_service_;
  std::function<base::Time()> now_;
  std::function<void(base::TimeDelta, std::function<void()>)>
      post_delayed_task_;
};

}  // namespace opcua
