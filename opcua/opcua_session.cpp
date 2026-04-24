#include "opcua/opcua_session.h"

#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "net/net_executor_adapter.h"
#include "opcua/opcua_subscription.h"
#include "scada/status_exception.h"
#include "transport/transport_factory.h"
#include "transport/transport_string.h"

#include <cctype>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

namespace {

constexpr std::size_t kOpcTcpPrefixLen = 9;  // "opc.tcp://"

}  // namespace

// static
OpcUaSession::ParsedEndpoint OpcUaSession::ParseEndpointUrl(
    const std::string& url) {
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

OpcUaSession::OpcUaSession(std::shared_ptr<Executor> executor,
                           transport::TransportFactory& transport_factory)
    : executor_{std::move(executor)},
      any_executor_{MakeAnyExecutor(executor_)},
      transport_factory_{transport_factory} {}

OpcUaSession::~OpcUaSession() = default;

promise<void> OpcUaSession::Connect(
    const scada::SessionConnectParams& params) {
  return ToPromise(NetExecutorAdapter{executor_}, ConnectAsync(params));
}

promise<void> OpcUaSession::Disconnect() {
  return ToPromise(NetExecutorAdapter{executor_}, DisconnectAsync());
}

promise<void> OpcUaSession::Reconnect() {
  return ToPromise(NetExecutorAdapter{executor_}, ReconnectAsync());
}

Awaitable<void> OpcUaSession::ConnectAsync(scada::SessionConnectParams params) {
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
  transport_ = std::make_unique<opcua::OpcUaBinaryClientTransport>(
      opcua::OpcUaBinaryClientTransportContext{
          .transport = std::move(*transport_result),
          .endpoint_url = endpoint_url_,
          .limits = {},
      });
  secure_channel_ =
      std::make_unique<opcua::OpcUaBinaryClientSecureChannel>(*transport_);
  channel_ = std::make_unique<opcua::OpcUaBinaryClientChannel>(
      opcua::OpcUaBinaryClientChannel::Context{
          .transport = *transport_,
          .secure_channel = *secure_channel_,
      });
  session_ = std::make_unique<opcua::OpcUaBinaryClientSession>(
      opcua::OpcUaBinaryClientSession::Context{
          .transport = *transport_,
          .secure_channel = *secure_channel_,
          .channel = *channel_,
      });

  const auto status = co_await session_->Create();
  if (status.bad()) {
    Reset();
    NotifyStateChanged(false, status);
    throw scada::status_exception{status};
  }
  is_connected_ = true;
  NotifyStateChanged(true, scada::Status{scada::StatusCode::Good});
}

Awaitable<void> OpcUaSession::DisconnectAsync() {
  if (!session_) {
    co_return;
  }
  co_await session_->Close();
  Reset();
  NotifyStateChanged(false, scada::Status{scada::StatusCode::Good});
}

Awaitable<void> OpcUaSession::ReconnectAsync() {
  co_return;
}

bool OpcUaSession::IsConnected(base::TimeDelta* ping_delay) const {
  if (ping_delay) {
    *ping_delay = {};
  }
  return is_connected_;
}

bool OpcUaSession::HasPrivilege(scada::Privilege) const {
  return true;
}

scada::NodeId OpcUaSession::GetUserId() const {
  return {};
}

std::string OpcUaSession::GetHostName() const {
  return {};
}

boost::signals2::scoped_connection OpcUaSession::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& callback) {
  return boost::signals2::scoped_connection{
      session_state_changed_.connect(callback)};
}

scada::SessionDebugger* OpcUaSession::GetSessionDebugger() {
  return nullptr;
}

void OpcUaSession::Browse(const scada::ServiceContext& context,
                          const std::vector<scada::BrowseDescription>& nodes,
                          const scada::BrowseCallback& callback) {
  auto weak_self = weak_from_this();
  CoSpawn(any_executor_, [this, context, inputs = nodes, callback,
                          weak_self]() mutable -> Awaitable<void> {
    auto self = weak_self.lock();
    if (!self) {
      co_return;
    }
    auto [status, results] = co_await Browse(std::move(context),
                                             std::move(inputs));
    callback(std::move(status), std::move(results));
  });
}

void OpcUaSession::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  auto weak_self = weak_from_this();
  CoSpawn(any_executor_, [this, inputs = browse_paths, callback,
                          weak_self]() mutable -> Awaitable<void> {
    auto self = weak_self.lock();
    if (!self) {
      co_return;
    }
    auto [status, results] = co_await TranslateBrowsePaths(std::move(inputs));
    callback(std::move(status), std::move(results));
  });
}

std::shared_ptr<scada::MonitoredItem> OpcUaSession::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params) {
  assert(!read_value_id.node_id.is_null());
  return GetDefaultSubscription().CreateMonitoredItem(read_value_id, params);
}

void OpcUaSession::Read(
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  auto weak_self = weak_from_this();
  CoSpawn(any_executor_, [this, context, inputs, callback,
                          weak_self]() mutable -> Awaitable<void> {
    auto self = weak_self.lock();
    if (!self) {
      co_return;
    }
    auto [status, results] = co_await Read(std::move(context), inputs);
    callback(std::move(status), std::move(results));
  });
}

void OpcUaSession::Write(
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  auto weak_self = weak_from_this();
  CoSpawn(any_executor_, [this, context, inputs, callback,
                          weak_self]() mutable -> Awaitable<void> {
    auto self = weak_self.lock();
    if (!self) {
      co_return;
    }
    auto [status, results] = co_await Write(std::move(context), inputs);
    callback(std::move(status), std::move(results));
  });
}

void OpcUaSession::Call(const scada::NodeId& node_id,
                        const scada::NodeId& method_id,
                        const std::vector<scada::Variant>& arguments,
                        const scada::NodeId& user_id,
                        const scada::StatusCallback& callback) {
  auto weak_self = weak_from_this();
  CoSpawn(any_executor_,
          [this, node_id, method_id, args = arguments, user_id, callback,
           weak_self]() mutable -> Awaitable<void> {
            auto self = weak_self.lock();
            if (!self) {
              co_return;
            }
            auto status = co_await Call(std::move(node_id),
                                        std::move(method_id), std::move(args),
                                        std::move(user_id));
            callback(std::move(status));
          });
}

Awaitable<std::tuple<scada::Status, std::vector<scada::BrowseResult>>>
OpcUaSession::Browse(scada::ServiceContext /*context*/,
                     std::vector<scada::BrowseDescription> inputs) {
  if (!is_connected_) {
    co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                         std::vector<scada::BrowseResult>{}};
  }
  assert(session_);
  auto* session = session_.get();
  auto result = co_await session->Browse(std::move(inputs));
  if (result.ok()) {
    co_return std::tuple{scada::Status{scada::StatusCode::Good},
                         std::move(*result)};
  }
  co_return std::tuple{result.status(), std::vector<scada::BrowseResult>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::BrowsePathResult>>>
OpcUaSession::TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) {
  if (!is_connected_) {
    co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                         std::vector<scada::BrowsePathResult>{}};
  }
  assert(session_);
  auto* session = session_.get();
  auto result =
      co_await session->TranslateBrowsePathsToNodeIds(std::move(inputs));
  if (result.ok()) {
    co_return std::tuple{scada::Status{scada::StatusCode::Good},
                         std::move(*result)};
  }
  co_return std::tuple{result.status(),
                       std::vector<scada::BrowsePathResult>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::DataValue>>>
OpcUaSession::Read(
    scada::ServiceContext /*context*/,
    std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) {
  if (!is_connected_) {
    co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                         std::vector<scada::DataValue>{}};
  }
  assert(session_);
  auto* session = session_.get();
  auto copy = *inputs;
  auto result = co_await session->Read(std::move(copy));
  if (result.ok()) {
    co_return std::tuple{scada::Status{scada::StatusCode::Good},
                         std::move(*result)};
  }
  co_return std::tuple{result.status(), std::vector<scada::DataValue>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::StatusCode>>>
OpcUaSession::Write(
    scada::ServiceContext /*context*/,
    std::shared_ptr<const std::vector<scada::WriteValue>> inputs) {
  if (!is_connected_) {
    co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                         std::vector<scada::StatusCode>{}};
  }
  assert(session_);
  auto* session = session_.get();
  auto copy = *inputs;
  auto result = co_await session->Write(std::move(copy));
  if (result.ok()) {
    co_return std::tuple{scada::Status{scada::StatusCode::Good},
                         std::move(*result)};
  }
  co_return std::tuple{result.status(), std::vector<scada::StatusCode>{}};
}

Awaitable<scada::Status> OpcUaSession::Call(
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

OpcUaSubscription& OpcUaSession::GetDefaultSubscription() {
  if (!default_subscription_) {
    default_subscription_ = OpcUaSubscription::Create(*this);
  }
  return *default_subscription_;
}

void OpcUaSession::Reset() {
  default_subscription_.reset();
  session_.reset();
  channel_.reset();
  secure_channel_.reset();
  transport_.reset();
  is_connected_ = false;
}

void OpcUaSession::NotifyStateChanged(bool connected, scada::Status status) {
  session_state_changed_(connected, status);
}
