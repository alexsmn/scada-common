#pragma once

#include <map>
#include <memory>

#include "core/configuration_types.h"
#include "remote/message_sender.h"
#include "remote/subscription.h"

namespace protocol {
class Message;
class Request;
class Response;
}  // namespace protocol

namespace scada {
class AttributeService;
class EventService;
class HistoryService;
class MethodService;
class NodeManagementService;
class MonitoredItemService;
class ViewService;
}  // namespace scada

class Connection;
class Logger;
class NodeManagementStub;
class EventServiceStub;
class HistoryStub;
class SessionStub;
class SubscriptionStub;
class ViewServiceStub;

struct SessionContext {
  const std::shared_ptr<Logger> logger_;
  scada::NodeManagementService& node_management_service_;
  scada::AttributeService& attribute_service_;
  scada::MethodService& method_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::EventService& event_service_;
  scada::ViewService& view_service_;
  scada::HistoryService& history_service_;
  const scada::NodeId user_id_;
};

class SessionStub : public std::enable_shared_from_this<SessionStub>,
                    private MessageSender,
                    private SessionContext {
 public:
  virtual ~SessionStub();

  static std::shared_ptr<SessionStub> Create(SessionContext&& context);

  Connection* connection() { return connection_; }
  void SetConnection(Connection* connection);

  Logger& logger() { return *logger_; }

  const std::string& name() const { return name_; }
  const scada::NodeId& user_id() const { return user_id_; }

  void ProcessMessage(const protocol::Message& message);

  void OnSessionDeleted();

 private:
  explicit SessionStub(SessionContext&& context);

  void ProcessRequest(const protocol::Request& request);

  void OnCreateSubscription(int request_id);
  void OnDeleteSubscription(int request_id, int subscription_id);
  void OnCreateMonitoredItem(int request_id,
                             int subscription_id,
                             const scada::ReadValueId& read_value_id);
  void OnDeleteMonitoredItem(int request_id,
                             int subscription_id,
                             int monitored_item_id);

  void OnRead(unsigned request_id,
              const std::vector<scada::ReadValueId>& read_value_ids);
  void OnWrite(unsigned request_id,
               const scada::NodeId& item_id,
               double value,
               unsigned flags);
  void OnCall(unsigned request_id,
              const scada::NodeId& node_id,
              const scada::NodeId& method_id,
              const std::vector<scada::Variant>& arguments);

  void SendResponse(protocol::Response& response);

  // MessageSender
  virtual void Send(protocol::Message& message) override;
  virtual void Request(protocol::Request& request,
                       ResponseHandler response_handler) {}

  std::string name_;

  Connection* connection_;

  std::unique_ptr<ViewServiceStub> view_service_stub_;
  std::unique_ptr<NodeManagementStub> node_management_stub_;
  std::unique_ptr<HistoryStub> history_stub_;

  int next_subscription_id_ = 1;
  std::map<int /*subscription_id*/, std::unique_ptr<SubscriptionStub>>
      subscriptions_;

  std::unique_ptr<protocol::Message> send_message_;
};
