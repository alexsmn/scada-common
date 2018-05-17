#pragma once

#include "base/nested_logger.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "core/attribute_service.h"
#include "core/configuration_types.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"
#include "core/privileges.h"
#include "core/session_service.h"
#include "core/status.h"
#include "net/transport.h"
#include "remote/message_sender.h"

#include <map>

namespace base {
class SingleThreadTaskRunner;
}

namespace net {
class Logger;
class TransportFactory;
}  // namespace net

namespace scada {
class NodeManagementService;
class EventService;
class HistoryService;
class ViewService;
}  // namespace scada

class EventServiceProxy;
class Logger;
class NodeManagementProxy;
class HistoryProxy;
class SubscriptionProxy;
class ViewServiceProxy;

struct SessionProxyContext {
  const std::shared_ptr<Logger> logger_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  net::TransportFactory& transport_factory_;
};

class SessionProxy : private SessionProxyContext,
                     public scada::AttributeService,
                     public scada::MethodService,
                     public scada::MonitoredItemService,
                     public scada::SessionService,
                     public MessageSender,
                     private net::Transport::Delegate {
 public:
  explicit SessionProxy(SessionProxyContext&& context);
  virtual ~SessionProxy();

  scada::NodeManagementService& GetNodeManagementService();
  scada::EventService& GetEventService();
  scada::HistoryService& GetHistoryService();
  scada::ViewService& GetViewService();

  // scada::SessionService
  virtual void Connect(const std::string& connection_string,
                       const scada::LocalizedText& user_name,
                       const std::string& password,
                       bool allow_remote_logoff,
                       const scada::StatusCallback& callback) override;
  virtual void Reconnect() override;
  virtual bool IsConnected(base::TimeDelta* ping_delay) const override;
  virtual bool HasPrivilege(scada::Privilege privilege) const override;
  virtual bool IsScada() const override { return false; }
  virtual scada::NodeId GetUserId() const override;
  virtual std::string GetHostName() const override;
  virtual void AddObserver(scada::SessionStateObserver& observer) override;
  virtual void RemoveObserver(scada::SessionStateObserver& observer) override;

  // MessageSender
  virtual void Send(protocol::Message& message) override;
  virtual void Request(protocol::Request& request,
                       ResponseHandler response_handler) override;

  // scada::MonitoredItemService
  virtual std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id) override;

  // scada::AttributeService
  virtual void Read(const std::vector<scada::ReadValueId>& value_ids,
                    const scada::ReadCallback& callback) override;
  virtual void Write(const scada::NodeId& node_id,
                     double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const scada::StatusCallback& callback) override;

  // scada::MethodService
  virtual void Call(const scada::NodeId& node_id,
                    const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) override;

 protected:
  Logger& logger() { return *logger_; }

  // net::Transport::Delegate
  virtual void OnTransportOpened() override;
  virtual void OnTransportClosed(net::Error error) override;
  virtual void OnTransportMessageReceived(const void* data,
                                          size_t size) override;

 private:
  friend class EventServiceProxy;

  void Connect();
  void Disconnect();

  void OnSessionError(const scada::Status& status);

  void OnSessionCreated();
  void OnSessionDeleted();

  void OnMessageReceived(protocol::Message&& message);
  void OnCreateSessionResult(const protocol::Response& response);

  void ForwardConnectResult(const scada::Status& status);

  void Ping();

  std::unique_ptr<net::Logger> transport_logger_;
  std::unique_ptr<net::Transport> transport_;

  bool session_created_ = false;

  scada::LocalizedText user_name_;
  std::string password_;
  std::string host_;
  bool allow_remote_logoff_ = false;

  scada::NodeId user_node_id_;
  unsigned user_rights_ = 0;
  std::string session_token_;

  std::unique_ptr<SubscriptionProxy> subscription_;
  std::unique_ptr<ViewServiceProxy> view_service_proxy_;
  std::unique_ptr<NodeManagementProxy> node_management_proxy_;
  std::unique_ptr<EventServiceProxy> event_service_proxy_;
  std::unique_ptr<HistoryProxy> history_proxy_;

  std::map<int /*request_id*/, ResponseHandler> requests_;

  base::ObserverList<scada::SessionStateObserver> observers_;

  scada::StatusCallback connect_callback_;

  int next_request_id_ = 1;

  base::Timer ping_timer_;
  base::TimeTicks ping_time_;
  base::TimeDelta last_ping_delay_;
};
