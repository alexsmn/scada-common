#include "opcua/client_session.h"

#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "net/net_executor_adapter.h"
#include "opcua/client_subscription.h"
#include "scada/status_exception.h"
#include "transport/transport_factory.h"
#include "transport/transport_string.h"

#include <cctype>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace {

constexpr std::size_t kOpcTcpPrefixLen = 10;  // "opc.tcp://"

}  // namespace

namespace opcua {

namespace {

template <typename Handler>
void DispatchLegacyCallback(const AnyExecutor& executor,
                            std::weak_ptr<ClientSession> weak_self,
                            Handler handler) {
  CoSpawn(executor,
          [weak_self = std::move(weak_self),
           handler = std::move(handler)]() mutable -> Awaitable<void> {
            auto self = weak_self.lock();
            if (!self) {
              co_return;
            }
            co_await handler(*self);
          });
}

}  // namespace

class ClientSession::CoroutineSessionAdapter final
    : public scada::CoroutineSessionService {
 public:
  explicit CoroutineSessionAdapter(ClientSession& session)
      : session_{session} {}

  Awaitable<void> Connect(scada::SessionConnectParams params) override {
    co_await session_.ConnectAsync(std::move(params));
  }

  Awaitable<void> Reconnect() override {
    co_await session_.ReconnectAsync();
  }

  Awaitable<void> Disconnect() override {
    co_await session_.DisconnectAsync();
  }

  bool IsConnected(base::TimeDelta* ping_delay = nullptr) const override {
    return session_.IsConnected(ping_delay);
  }

  scada::NodeId GetUserId() const override {
    return session_.GetUserId();
  }

  bool HasPrivilege(scada::Privilege privilege) const override {
    return session_.HasPrivilege(privilege);
  }

  std::string GetHostName() const override {
    return session_.GetHostName();
  }

  bool IsScada() const override {
    return session_.IsScada();
  }

  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override {
    return session_.SubscribeSessionStateChanged(callback);
  }

  scada::SessionDebugger* GetSessionDebugger() override {
    return session_.GetSessionDebugger();
  }

 private:
  ClientSession& session_;
};

// static
ClientSession::ParsedEndpoint
ClientSession::ParseEndpointUrl(const std::string& url) {
  ParsedEndpoint result;
  // "opc.tcp://host[:port][/path]". Any /path suffix is discarded (the
  // server endpoint name is tracked separately by CreateSession's
  // ClientDescription.ApplicationUri).
  if (url.size() <= kOpcTcpPrefixLen ||
      url.compare(0, kOpcTcpPrefixLen, "opc.tcp://") != 0) {
    return result;
  }
  const auto authority_start = kOpcTcpPrefixLen;
  const auto path_pos = url.find('/', authority_start);
  const std::string authority =
      url.substr(authority_start,
                 path_pos == std::string::npos
                     ? std::string::npos
                     : path_pos - authority_start);
  const auto colon_pos = authority.rfind(':');
  if (colon_pos == std::string::npos) {
    result.host = authority;
    result.valid = !result.host.empty();
    return result;
  }
  result.host = authority.substr(0, colon_pos);
  const auto port_str = authority.substr(colon_pos + 1);
  if (result.host.empty() || port_str.empty()) {
    return result;
  }
  int port = 0;
  for (char c : port_str) {
    if (!std::isdigit(static_cast<unsigned char>(c))) {
      return result;
    }
    port = port * 10 + (c - '0');
  }
  result.port = port;
  result.valid = true;
  return result;
}

ClientSession::ClientSession(
    std::shared_ptr<Executor> executor,
    transport::TransportFactory& transport_factory)
    : executor_{std::move(executor)},
      any_executor_{MakeAnyExecutor(executor_)},
      transport_factory_{transport_factory} {
  coroutine_session_service_ = std::make_unique<CoroutineSessionAdapter>(*this);
}

ClientSession::~ClientSession() = default;

promise<void> ClientSession::Connect(
    const scada::SessionConnectParams& params) {
  return ToPromise(NetExecutorAdapter{executor_}, ConnectAsync(params));
}

promise<void> ClientSession::Disconnect() {
  return ToPromise(NetExecutorAdapter{executor_}, DisconnectAsync());
}

promise<void> ClientSession::Reconnect() {
  return ToPromise(NetExecutorAdapter{executor_}, ReconnectAsync());
}

Awaitable<void> ClientSession::ConnectAsync(
    scada::SessionConnectParams params) {
  // Use `connection_string` when populated, otherwise compose from `host`.
  std::string endpoint = params.connection_string.empty()
                             ? std::string{"opc.tcp://"} + params.host
                             : params.connection_string;
  const auto parsed = ParseEndpointUrl(endpoint);
  if (!parsed.valid) {
    throw scada::status_exception{scada::StatusCode::Bad};
  }

  transport::TransportString ts;
  ts.SetProtocol(transport::TransportString::TCP);
  ts.SetActive(true);
  ts.SetParam(transport::TransportString::kParamHost, parsed.host);
  ts.SetParam(transport::TransportString::kParamPort, parsed.port);

  const transport::executor net_executor{NetExecutorAdapter{executor_}};
  auto transport_result = transport_factory_.CreateTransport(
      ts, net_executor, transport::log_source{});
  if (!transport_result.ok()) {
    throw scada::status_exception{
        scada::Status{scada::StatusCode::Bad_Disconnected}};
  }

  endpoint_url_ = std::move(endpoint);
  transport_ = std::make_unique<binary::ClientTransport>(
      binary::ClientTransportContext{
          .transport = std::move(*transport_result),
          .endpoint_url = endpoint_url_,
          .limits = {},
      });
  secure_channel_ =
      std::make_unique<binary::ClientSecureChannel>(*transport_);
  connection_ = std::make_unique<binary::ClientConnection>(
      binary::ClientConnection::Context{
          .transport = *transport_,
          .secure_channel = *secure_channel_,
      });
  channel_ = std::make_unique<ClientChannel>(
      ClientChannel::Context{
          .connection = *connection_,
      });
  session_ = std::make_unique<ClientProtocolSession>(
      ClientProtocolSession::Context{
          .connection = *connection_,
          .channel = *channel_,
      });

  ClientProtocolSession::Identity identity;
  if (!params.user_name.empty()) {
    identity.user_name = params.user_name;
    identity.password = params.password;
  }

  const auto status = co_await session_->Create(
      base::TimeDelta::FromMinutes(10), std::move(identity));
  if (status.bad()) {
    Reset();
    NotifyStateChanged(false, status);
    throw scada::status_exception{status};
  }
  is_connected_ = true;
  NotifyStateChanged(true, scada::Status{scada::StatusCode::Good});
}

Awaitable<void> ClientSession::DisconnectAsync() {
  if (!session_) {
    co_return;
  }
  co_await session_->Close();
  Reset();
  NotifyStateChanged(false, scada::Status{scada::StatusCode::Good});
}

Awaitable<void> ClientSession::ReconnectAsync() {
  co_return;
}

scada::CoroutineSessionService& ClientSession::coroutine_session_service() {
  return *coroutine_session_service_;
}

bool ClientSession::IsConnected(base::TimeDelta* ping_delay) const {
  if (ping_delay) {
    *ping_delay = {};
  }
  return is_connected_;
}

bool ClientSession::HasPrivilege(scada::Privilege) const {
  return true;
}

scada::NodeId ClientSession::GetUserId() const {
  return {};
}

std::string ClientSession::GetHostName() const {
  return {};
}

boost::signals2::scoped_connection ClientSession::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& callback) {
  return boost::signals2::scoped_connection{
      session_state_changed_.connect(callback)};
}

scada::SessionDebugger* ClientSession::GetSessionDebugger() {
  return nullptr;
}

void ClientSession::Browse(const scada::ServiceContext& context,
                          const std::vector<scada::BrowseDescription>& nodes,
                          const scada::BrowseCallback& callback) {
  DispatchLegacyCallback(
      any_executor_, weak_from_this(),
      [context, inputs = nodes, callback](
          ClientSession& self) mutable -> Awaitable<void> {
        auto result = co_await self.Browse(std::move(context),
                                           std::move(inputs));
        auto status = result.status();
        auto results = std::move(result).value_or({});
        callback(std::move(status), std::move(results));
      });
}

void ClientSession::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  DispatchLegacyCallback(
      any_executor_, weak_from_this(),
      [inputs = browse_paths, callback](
          ClientSession& self) mutable -> Awaitable<void> {
        auto result = co_await self.TranslateBrowsePaths(std::move(inputs));
        auto status = result.status();
        auto results = std::move(result).value_or({});
        callback(std::move(status), std::move(results));
      });
}

std::shared_ptr<scada::MonitoredItem> ClientSession::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params) {
  assert(!read_value_id.node_id.is_null());
  return GetDefaultSubscription().CreateMonitoredItem(read_value_id, params);
}

void ClientSession::Read(
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  DispatchLegacyCallback(
      any_executor_, weak_from_this(),
      [context, inputs, callback](
          ClientSession& self) mutable -> Awaitable<void> {
        auto result = co_await self.Read(std::move(context), inputs);
        auto status = result.status();
        auto results = std::move(result).value_or({});
        callback(std::move(status), std::move(results));
      });
}

void ClientSession::Write(
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  DispatchLegacyCallback(
      any_executor_, weak_from_this(),
      [context, inputs, callback](
          ClientSession& self) mutable -> Awaitable<void> {
        auto result = co_await self.Write(std::move(context), inputs);
        auto status = result.status();
        auto results = std::move(result).value_or({});
        callback(std::move(status), std::move(results));
      });
}

void ClientSession::Call(const scada::NodeId& node_id,
                        const scada::NodeId& method_id,
                        const std::vector<scada::Variant>& arguments,
                        const scada::NodeId& user_id,
                        const scada::StatusCallback& callback) {
  DispatchLegacyCallback(
      any_executor_, weak_from_this(),
      [node_id, method_id, args = arguments, user_id, callback](
          ClientSession& self) mutable -> Awaitable<void> {
        auto status = co_await self.Call(std::move(node_id),
                                         std::move(method_id), std::move(args),
                                         std::move(user_id));
        callback(std::move(status));
      });
}

Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
ClientSession::Browse(scada::ServiceContext /*context*/,
                     std::vector<scada::BrowseDescription> inputs) {
  if (!is_connected_) {
    co_return scada::Status{scada::StatusCode::Bad_Disconnected};
  }
  assert(session_);
  auto* session = session_.get();
  auto result = co_await session->Browse(std::move(inputs));
  if (result.ok()) {
    co_return std::move(*result);
  }
  co_return result.status();
}

Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
ClientSession::TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) {
  if (!is_connected_) {
    co_return scada::Status{scada::StatusCode::Bad_Disconnected};
  }
  assert(session_);
  auto* session = session_.get();
  auto result =
      co_await session->TranslateBrowsePathsToNodeIds(std::move(inputs));
  if (result.ok()) {
    co_return std::move(*result);
  }
  co_return result.status();
}

Awaitable<scada::StatusOr<std::vector<scada::DataValue>>>
ClientSession::Read(
    scada::ServiceContext /*context*/,
    std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) {
  if (!is_connected_) {
    co_return scada::Status{scada::StatusCode::Bad_Disconnected};
  }
  assert(session_);
  auto* session = session_.get();
  auto copy = *inputs;
  auto result = co_await session->Read(std::move(copy));
  if (result.ok()) {
    co_return std::move(*result);
  }
  co_return result.status();
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientSession::Write(
    scada::ServiceContext /*context*/,
    std::shared_ptr<const std::vector<scada::WriteValue>> inputs) {
  if (!is_connected_) {
    co_return scada::Status{scada::StatusCode::Bad_Disconnected};
  }
  assert(session_);
  auto* session = session_.get();
  auto copy = *inputs;
  auto result = co_await session->Write(std::move(copy));
  if (result.ok()) {
    co_return std::move(*result);
  }
  co_return result.status();
}

Awaitable<scada::Status> ClientSession::Call(
    scada::NodeId node_id,
    scada::NodeId method_id,
    std::vector<scada::Variant> arguments,
    scada::NodeId /*user_id*/) {
  if (!is_connected_) {
    co_return scada::Status{scada::StatusCode::Bad_Disconnected};
  }
  assert(session_);
  auto* session = session_.get();
  auto result = co_await session->Call(std::move(node_id),
                                       std::move(method_id),
                                       std::move(arguments));
  if (result.ok()) {
    co_return result->status;
  }
  co_return result.status();
}

ClientSubscription& ClientSession::GetDefaultSubscription() {
  if (!default_subscription_) {
    default_subscription_ = ClientSubscription::Create(*this);
  }
  return *default_subscription_;
}

void ClientSession::Reset() {
  default_subscription_.reset();
  session_.reset();
  channel_.reset();
  connection_.reset();
  secure_channel_.reset();
  transport_.reset();
  is_connected_ = false;
}

void ClientSession::NotifyStateChanged(bool connected,
                                            scada::Status status) {
  session_state_changed_(connected, status);
}

}  // namespace opcua
