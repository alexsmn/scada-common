#include "remote/session_proxy.h"

#include "base/strings/stringprintf.h"
#include "net/transport_factory.h"
#include "net/transport_string.h"
#include "core/monitored_item.h"
#include "core/status.h"
#include "core/write_flags.h"
#include "core/session_state_observer.h"
#include "remote/protocol.h"
#include "remote/protocol_message_reader.h"
#include "remote/protocol_message_transport.h"
#include "remote/protocol_utils.h"
#include "remote/event_service_proxy.h"
#include "remote/history_proxy.h"
#include "remote/node_management_proxy.h"
#include "remote/subscription_proxy.h"
#include "remote/view_service_proxy.h"

SessionProxy::SessionProxy(std::shared_ptr<Logger> logger, net::TransportFactory& transport_factory)
    : logger_{std::move(logger)},
      transport_factory_{transport_factory} {
  SubscriptionParams params;
  subscription_ = std::make_unique<SubscriptionProxy>(params);

  view_service_proxy_ = std::make_unique<ViewServiceProxy>(
      std::make_shared<NestedLogger>(logger_, "ViewService"));

  node_management_proxy_ = std::make_unique<NodeManagementProxy>(
      std::make_shared<NestedLogger>(logger_, "NodeManagementService"));

  event_service_proxy_ = std::make_unique<EventServiceProxy>();

  history_proxy_ = std::make_unique<HistoryProxy>();
}

SessionProxy::~SessionProxy() {
  if (session_created_) {
    protocol::Request request;
    auto& delete_session = *request.mutable_delete_session();
    Request(request, nullptr);
  }
}


void SessionProxy::OnTransportOpened() {
  logger().Write(LogSeverity::Normal, "Transport opened");

  protocol::Request request;
  auto& create_session = *request.mutable_create_session();
  create_session.set_user_name(user_name_);
  create_session.set_password(password_);
  create_session.set_protocol_version_major(protocol::PROTOCOL_VERSION_MAJOR);
  create_session.set_protocol_version_minor(protocol::PROTOCOL_VERSION_MINOR);
  if (allow_remote_logoff_)
    create_session.set_delete_existing(true);

  Request(request, [this](const protocol::Response& response) {
    OnCreateSessionResult(response);
  });
}

void SessionProxy::OnSessionCreated() {
  assert(!session_created_);
  session_created_ = true;

  subscription_->OnChannelOpened(*this);
  view_service_proxy_->OnChannelOpened(*this);
  node_management_proxy_->OnChannelOpened(*this);
  event_service_proxy_->OnChannelOpened(*this);
  history_proxy_->OnChannelOpened(*this);

  FOR_EACH_OBSERVER(scada::SessionStateObserver, observers_, OnSessionCreated());

  ForwardConnectResult(scada::StatusCode::Good);
}

void SessionProxy::OnTransportClosed(net::Error error) {
  logger().WriteF(LogSeverity::Warning, "Transport closed as %s", ErrorToString(error));

  scada::Status status_code(
      error == net::OK ? scada::StatusCode::Bad_SessionForcedLogoff :
      error == net::ERR_SSL_BAD_PEER_PUBLIC_KEY ? scada::StatusCode::Bad_UserIsAlreadyLoggedOn :
      scada::StatusCode::Bad_Disconnected);
  OnSessionError(status_code);
}

void SessionProxy::OnSessionError(const scada::Status& status) {
  transport_.reset();

  OnSessionDeleted();

  ForwardConnectResult(status);

  FOR_EACH_OBSERVER(scada::SessionStateObserver, observers_, OnSessionDeleted(status));
}

void SessionProxy::OnTransportMessageReceived(const void* data, size_t size) {
  protocol::Message m;
  if (!m.ParseFromArray(data, size))
    throw E_FAIL;
  OnMessageReceived(m);
}

void SessionProxy::OnSessionDeleted() {
  // When OnSessionCloased is called on connection, channel actually is still
  // not opened.
  if (!session_created_)
    return;

  session_created_ = false;

  // Cancel all requests.
  {
    protocol::Response response;
    ToProto(scada::Status(scada::StatusCode::Bad_Disconnected), *response.add_status());
    auto requests = std::move(requests_);
    for (auto& p : requests) {
      response.set_request_id(p.first);
      if (p.second)
        p.second(response);
    }
  }

  subscription_->OnChannelClosed();
  view_service_proxy_->OnChannelClosed();
  node_management_proxy_->OnChannelClosed();
  event_service_proxy_->OnChannelClosed();
  history_proxy_->OnChannelClosed();
}

bool SessionProxy::has_privilege(cfg::Privilege privilege) const {
  return (user_rights_ & (1 << privilege)) != 0;
}

scada::NodeManagementService& SessionProxy::GetNodeManagementService() {
  return *node_management_proxy_;
}

scada::EventService& SessionProxy::GetEventService() {
  return *event_service_proxy_;
}

scada::HistoryService& SessionProxy::GetHistoryService() {
  return *history_proxy_;
}

scada::ViewService& SessionProxy::GetViewService() {
  return *view_service_proxy_;
}

bool SessionProxy::IsAdministrator() const {
  return has_privilege(cfg::PRIVILEGE_CONFIGURE);
}

void SessionProxy::AddObserver(scada::SessionStateObserver& observer) {
  observers_.AddObserver(&observer);
}

void SessionProxy::RemoveObserver(scada::SessionStateObserver& observer) {
  observers_.RemoveObserver(&observer);
}

void SessionProxy::Send(protocol::Message& message) {
  assert(message.IsInitialized());

  // TODO: This check shall be changed on assert when all proxy object will
  // support MonitoredItemService reconnection.
  if (!transport_.get()) {
    assert(false);
    return;
  }

  std::string string;
  if (!message.AppendToString(&string))
    throw E_FAIL;
  transport_->Write(string.data(), string.size());
}

void SessionProxy::OnMessageReceived(const protocol::Message& message) {
//  OutputDebugStringA(message.DebugString().c_str());
//  OutputDebugStringA("\n");

  for (auto& response : message.responses()) {
    auto i = requests_.find(response.request_id());
    if (i != requests_.end()) {
      auto handler = std::move(i->second);
      requests_.erase(i);
      handler(response);
    }
  }

  for (auto& notification : message.notifications()) {
    for (auto& data_change : notification.data_changes()) {
      subscription_->OnDataChange(
          data_change.monitored_item_id(),
          FromProto(data_change.data_value()));
    }

    view_service_proxy_->OnNotification(notification);

    for (auto& event : notification.events())
      subscription_->OnEvent(event.monitored_item_id(), FromProto(event));

    if (notification.has_session_deleted()) {
      OnSessionError(scada::StatusCode::Bad_ServerWasShutDown);
      return;
    }
  }
}

void SessionProxy::Request(protocol::Request& request, ResponseHandler response_handler) {
  auto request_id = next_request_id_;
  next_request_id_ = next_request_id_ == std::numeric_limits<int>::max() ? 1 : (next_request_id_ + 1);

  if (response_handler)
    requests_[request_id] = std::move(response_handler);

  request.set_request_id(request_id);
  assert(request.IsInitialized());

  protocol::Message message;
  message.add_requests()->Swap(&request);
  Send(message);
}

void SessionProxy::Connect(const std::string& host,
                           const std::string& username, const std::string& password,
                           bool allow_remote_logoff, ConnectCallback callback) {
  assert(!transport_);

  user_name_ = username;
  password_ = password;
  host_ = host;
  allow_remote_logoff_ = allow_remote_logoff;
  connect_callback_ = std::move(callback);

  std::string connection_string =
      base::StringPrintf("Host=%s;Port=2000", host_.c_str());

  logger().WriteF(LogSeverity::Normal, "Connecting as '%s' to '%s'",
      user_name_.c_str(), connection_string.c_str());

  auto transport = transport_factory_.CreateTransport(net::TransportString(connection_string));
  if (!transport) {
    OnTransportClosed(net::ERR_FAILED);
    return;
  }

  transport_.reset(new ProtocolMessageTransport(std::move(transport)));
  transport_->set_delegate(this);

  net::Error error = transport_->Open();
  if (error != net::OK)
    OnTransportClosed(error);
}

void SessionProxy::ForwardConnectResult(const scada::Status& status) {
  auto callback = std::move(connect_callback_);
  if (callback)
    callback(status);
}

void SessionProxy::OnCreateSessionResult(const protocol::Response& response) {
  auto status = FromProto(response.status(0));
  if (!status) {
    OnSessionError(status);
    return;
  }

  if (!response.has_create_session_result()) {
    OnSessionError(scada::StatusCode::Bad);
    return;
  }

  auto& create_session_result = response.create_session_result();
  session_token_ = create_session_result.token();
  user_node_id_ = FromProto(create_session_result.user_node_id());
  user_rights_ = create_session_result.user_rights();

  OnSessionCreated();
}

void SessionProxy::Read(const std::vector<scada::ReadValueId>& value_ids, const scada::ReadCallback& callback) {
  if (!session_created_) {
    callback(scada::StatusCode::Bad_Disconnected, {});
    return;
  }

  protocol::Request request;
  auto& read = *request.mutable_read();
  for (auto& value_id : value_ids)
    ToProto(value_id, *read.add_value_id());

  Request(request,
      [this, callback](const protocol::Response& response) {
        if (callback)
          callback(FromProto(response.status(0)), VectorFromProto<scada::DataValue>(response.read().result()));
      });
}

void SessionProxy::Write(const scada::NodeId& item_id, double value,
                         const scada::NodeId& user_id, const scada::WriteFlags& flags,
                         const scada::StatusCallback& callback) {
  if (!session_created_) {
    callback(scada::StatusCode::Bad_Disconnected);
    return;
  }

  protocol::Request request;
  auto& write = *request.mutable_write();
  ToProto(item_id, *write.mutable_node_id());
  write.set_value(value);
  if (flags.select())
    write.set_select(true);

  Request(request,
      [this, callback](const protocol::Response& response) {
        if (callback)
          callback(FromProto(response.status(0)));
      });
}

void SessionProxy::Call(const scada::NodeId& node_id, const scada::NodeId& method_id,
                        const std::vector<scada::Variant>& arguments,
                        const scada::StatusCallback& callback) {
  if (!session_created_) {
    callback(scada::StatusCode::Bad_Disconnected);
    return;
  }

  protocol::Request request;
  auto& command = *request.mutable_call()->mutable_device_command();
  ToProto(node_id, *command.mutable_node_id());
  ToProto(method_id, *command.mutable_method_id());
  ContainerToProto(arguments, *command.mutable_argument());

  Request(request,
      [this, callback](const protocol::Response& response) {
        if (callback)
          callback(FromProto(response.status(0)));
      });
}

std::unique_ptr<scada::MonitoredItem> SessionProxy::CreateMonitoredItem(const scada::NodeId& node_id, scada::AttributeId attribute_id) {
  return subscription_->CreateMonitoredItem(node_id, attribute_id);
}

bool SessionProxy::IsConnected() const {
  return session_created_;
}

scada::NodeId SessionProxy::GetUserId() const {
  return user_node_id_;
}

std::string SessionProxy::GetHostName() const {
  return host_;
}
