#include "remote/session_stub.h"

#include "core/attribute_service.h"
#include "core/event_service.h"
#include "core/method_service.h"
#include "core/write_flags.h"
#include "remote/connection.h"
#include "remote/history_stub.h"
#include "remote/node_management_stub.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"
#include "remote/subscription_stub.h"
#include "remote/view_service_stub.h"

SessionStub::SessionStub(SessionContext&& context)
    : SessionContext(std::move(context)),
      // Can't use std::make_unique:
      // conversion from 'SessionStub *' to 'MessageSender &' exists, but is
      // inaccessible
      view_service_stub_{new ViewServiceStub{
          ViewServiceStubContext{logger_, io_context_, *this, view_service_}}},
      node_management_stub_{new NodeManagementStub{
          *this, node_management_service_, user_id_, logger_}},
      history_stub_{
          new HistoryStub{history_service_, *this, io_context_, logger_}} {
  logger().Write(LogSeverity::Normal, "Created");
}

SessionStub::~SessionStub() {
  logger().Write(LogSeverity::Normal, "Destroying");

  if (connection_)
    connection_->OnSessionDeleted();
}

std::shared_ptr<SessionStub> SessionStub::Create(SessionContext&& context) {
  return std::shared_ptr<SessionStub>(new SessionStub(std::move(context)));
}

void SessionStub::SetConnection(Connection* connection) {
  connection_ = connection;

  if (connection_ && send_message_) {
    auto message = std::move(send_message_);
    connection_->Send(*message);
  }
}

void SessionStub::ProcessMessage(const protocol::Message& message) {
  for (auto& request : message.requests())
    ProcessRequest(request);
}

void SessionStub::ProcessRequest(const protocol::Request& request) {
  if (request.has_ping()) {
    protocol::Response response;
    response.set_request_id(request.request_id());
    ToProto(scada::StatusCode::Good, *response.mutable_status());
    SendResponse(response);
  }

  if (request.has_read()) {
    auto& read = request.read();
    OnRead(request.request_id(),
           VectorFromProto<scada::ReadValueId>(read.value_id()));
  }

  if (request.has_write()) {
    auto& write = request.write();
    scada::WriteFlags flags;
    if (write.select())
      flags.set_select();
    OnWrite(request.request_id(), FromProto(write.node_id()), write.value(),
            flags.raw());
  }

  if (request.has_call()) {
    auto& call = request.call();
    if (call.has_acknowledge()) {
      auto& acknowledge = call.acknowledge();
      event_service_.Acknowledge(acknowledge.acknowledge_id(), user_id_);
      protocol::Response response;
      response.set_request_id(request.request_id());
      ToProto(scada::StatusCode::Good, *response.mutable_status());
      SendResponse(response);
    }
    if (call.has_device_command()) {
      auto& device_command = call.device_command();
      OnCall(request.request_id(), FromProto(device_command.node_id()),
             FromProto(device_command.method_id()),
             VectorFromProto<scada::Variant>(device_command.argument()));
    }
  }

  if (request.has_create_subscription()) {
    auto& create_subscription = request.create_subscription();
    OnCreateSubscription(request.request_id());
  }

  if (request.has_delete_subscription()) {
    auto& delete_subscription = request.delete_subscription();
    OnDeleteSubscription(request.has_create_subscription(),
                         delete_subscription.subscription_id());
  }

  if (request.has_create_monitored_item()) {
    auto& create_monitored_item = request.create_monitored_item();
    OnCreateMonitoredItem(request.request_id(),
                          create_monitored_item.subscription_id(),
                          {FromProto(create_monitored_item.node_id()),
                           static_cast<scada::AttributeId>(
                               create_monitored_item.attribute_id())});
  }

  if (request.has_delete_monitored_item()) {
    auto& delete_monitored_item = request.delete_monitored_item();
    OnDeleteMonitoredItem(request.request_id(),
                          delete_monitored_item.subscription_id(),
                          delete_monitored_item.monitored_item_id());
  }

  view_service_stub_->OnRequestReceived(request);
  node_management_stub_->OnRequestReceived(request);
  history_stub_->OnRequestReceived(request);
}

void SessionStub::Send(protocol::Message& message) {
  if (connection_)
    connection_->Send(message);
  else {
    if (send_message_) {
      send_message_->MergeFrom(message);
    } else {
      send_message_.reset(new protocol::Message);
      send_message_->Swap(&message);
    }
  }
}

void SessionStub::OnCreateSubscription(int request_id) {
  SubscriptionParams params;

  auto subscription_id = next_subscription_id_++;
  // Can't use std::make_unique:
  // conversion from 'SessionStub *' to 'MessageSender &' exists, but is
  // inaccessible
  std::unique_ptr<SubscriptionStub> subscription{new SubscriptionStub{
      *this, monitored_item_service_, subscription_id, params}};
  subscriptions_.emplace(subscription_id, std::move(subscription));

  protocol::Message message;
  auto& response = *message.add_responses();
  response.set_request_id(request_id);
  auto& create_subscription_result =
      *response.mutable_create_subscription_result();
  create_subscription_result.set_subscription_id(subscription_id);
  ToProto(scada::StatusCode::Good, *response.mutable_status());
  Send(message);
}

void SessionStub::OnDeleteSubscription(int request_id, int subscription_id) {
  auto result = subscriptions_.erase(subscription_id)
                    ? scada::StatusCode::Good
                    : scada::StatusCode::Bad_WrongSubscriptionId;

  protocol::Message message;
  auto& response = *message.add_responses();
  response.set_request_id(request_id);
  ToProto(result, *response.mutable_status());
  Send(message);
}

void SessionStub::OnCreateMonitoredItem(
    int request_id,
    int subscription_id,
    const scada::ReadValueId& read_value_id) {
  auto i = subscriptions_.find(subscription_id);
  if (i != subscriptions_.end())
    i->second->OnCreateMonitoredItem(request_id, read_value_id);
}

void SessionStub::OnDeleteMonitoredItem(int request_id,
                                        int subscription_id,
                                        int monitored_item_id) {
  auto i = subscriptions_.find(subscription_id);
  if (i != subscriptions_.end())
    i->second->OnDeleteMonitoredItem(request_id, monitored_item_id);
}

void SessionStub::SendResponse(protocol::Response& response) {
  protocol::Message message;
  message.add_responses()->Swap(&response);
  Send(message);
}

void SessionStub::OnSessionDeleted() {
  protocol::Message message;
  message.add_notifications()->mutable_session_deleted();
  Send(message);
}

void SessionStub::OnCall(unsigned request_id,
                         const scada::NodeId& node_id,
                         const scada::NodeId& method_id,
                         const std::vector<scada::Variant>& arguments) {
  std::weak_ptr<SessionStub> weak_ptr = shared_from_this();
  method_service_.Call(node_id, method_id, arguments, user_id_,
                       [weak_ptr, request_id](const scada::Status& status) {
                         auto ptr = weak_ptr.lock();
                         if (!ptr)
                           return;

                         protocol::Message message;
                         auto& response = *message.add_responses();
                         response.set_request_id(request_id);
                         ToProto(status, *response.mutable_status());
                         ptr->Send(message);
                       });
}

void SessionStub::OnRead(
    unsigned request_id,
    const std::vector<scada::ReadValueId>& read_value_ids) {
  std::weak_ptr<SessionStub> weak_ptr = shared_from_this();
  attribute_service_.Read(
      read_value_ids,
      [weak_ptr, request_id](scada::Status&& status,
                             std::vector<scada::DataValue>&& results) {
        auto ptr = weak_ptr.lock();
        if (!ptr)
          return;

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        ToProto(std::move(status), *response.mutable_status());
        ContainerToProto(std::move(results),
                         *response.mutable_read_result()->mutable_value());
        ptr->Send(message);
      });
}

void SessionStub::OnWrite(unsigned request_id,
                          const scada::NodeId& item_id,
                          double value,
                          unsigned flags) {
  std::weak_ptr<SessionStub> weak_ptr = shared_from_this();
  attribute_service_.Write(item_id, value, user_id_, scada::WriteFlags(flags),
                           [weak_ptr, request_id](const scada::Status& status) {
                             auto ptr = weak_ptr.lock();
                             if (!ptr)
                               return;

                             protocol::Message message;
                             auto& response = *message.add_responses();
                             response.set_request_id(request_id);
                             ToProto(status, *response.mutable_status());
                             ptr->Send(message);
                           });
}
