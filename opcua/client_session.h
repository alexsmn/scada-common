#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/executor.h"
#include "base/promise.h"
#include "opcua/binary/client_connection.h"
#include "opcua/binary/client_secure_channel.h"
#include "opcua/binary/client_transport.h"
#include "opcua/client_channel.h"
#include "opcua/client_protocol_session.h"
#include "scada/attribute_service.h"
#include "scada/coroutine_services.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "scada/session_service.h"
#include "scada/view_service.h"

#include <boost/signals2/signal.hpp>
#include <memory>
#include <tuple>

namespace transport {
class TransportFactory;
}  // namespace transport

namespace opcua {

class ClientSubscription;

// Qt client's adapter onto the in-repo OPC UA Binary client (see
// common/opcua/binary/*). Implements the five scada::* service
// interfaces and bridges their callback / promise calling conventions to
// the coroutine-native client stack underneath.
class ClientSession final
    : public std::enable_shared_from_this<ClientSession>,
      public scada::SessionService,
      public scada::CoroutineViewService,
      public scada::CoroutineAttributeService,
      public scada::CoroutineMethodService,
      public scada::ViewService,
      public scada::AttributeService,
      public scada::MonitoredItemService,
      public scada::MethodService {
 public:
  ClientSession(std::shared_ptr<Executor> executor,
                     transport::TransportFactory& transport_factory);
  ~ClientSession() override;

  // scada::SessionService
  promise<void> Connect(
      const scada::SessionConnectParams& params) override;
  promise<void> Disconnect() override;
  promise<void> Reconnect() override;
  bool IsConnected(
      base::TimeDelta* ping_delay = nullptr) const override;
  bool HasPrivilege(scada::Privilege privilege) const override;
  bool IsScada() const override { return false; }
  scada::NodeId GetUserId() const override;
  std::string GetHostName() const override;
  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override;
  scada::SessionDebugger* GetSessionDebugger() override;

  [[nodiscard]] Awaitable<void> ConnectAsync(
      scada::SessionConnectParams params);
  [[nodiscard]] Awaitable<void> DisconnectAsync();
  [[nodiscard]] Awaitable<void> ReconnectAsync();

  // scada::ViewService
  void Browse(const scada::ServiceContext& context,
              const std::vector<scada::BrowseDescription>& nodes,
              const scada::BrowseCallback& callback) override;
  void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

  // scada::MonitoredItemService
  std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id,
      const scada::MonitoringParameters& params) override;

  // scada::AttributeService
  void Read(
      const scada::ServiceContext& context,
      const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
      const scada::ReadCallback& callback) override;
  void Write(
      const scada::ServiceContext& context,
      const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
      const scada::WriteCallback& callback) override;

  // scada::MethodService
  void Call(const scada::NodeId& node_id,
            const scada::NodeId& method_id,
            const std::vector<scada::Variant>& arguments,
            const scada::NodeId& user_id,
            const scada::StatusCallback& callback) override;

  // scada::CoroutineViewService
  [[nodiscard]] Awaitable<
      std::tuple<scada::Status, std::vector<scada::BrowseResult>>>
  Browse(scada::ServiceContext context,
         std::vector<scada::BrowseDescription> inputs) override;
  [[nodiscard]] Awaitable<
      std::tuple<scada::Status, std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override;

  // scada::CoroutineAttributeService
  [[nodiscard]] Awaitable<
      std::tuple<scada::Status, std::vector<scada::DataValue>>>
  Read(scada::ServiceContext context,
       std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) override;
  [[nodiscard]] Awaitable<
      std::tuple<scada::Status, std::vector<scada::StatusCode>>>
  Write(scada::ServiceContext context,
        std::shared_ptr<const std::vector<scada::WriteValue>> inputs) override;

  // scada::CoroutineMethodService
  [[nodiscard]] Awaitable<scada::Status> Call(
      scada::NodeId node_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments,
      scada::NodeId user_id) override;

  // Access for ClientSubscription / MonitoredItem.
  [[nodiscard]] ClientChannel& channel() { return *channel_; }
  [[nodiscard]] const AnyExecutor& any_executor() const { return any_executor_; }
  [[nodiscard]] bool is_connected() const { return is_connected_; }

 private:
  // Internal teardown on error or Disconnect.
  void Reset();

  // Parses "opc.tcp://host:port" into a transport string acceptable to
  // TransportFactory (TCP;Active;Host=...;Port=...).
  struct ParsedEndpoint {
    std::string host;
    int port = 4840;
    bool valid = false;
  };
  static ParsedEndpoint ParseEndpointUrl(const std::string& url);

  void NotifyStateChanged(bool connected, scada::Status status);

  ClientSubscription& GetDefaultSubscription();

  const std::shared_ptr<Executor> executor_;
  const AnyExecutor any_executor_;
  transport::TransportFactory& transport_factory_;

  // Entire client stack is lazily constructed on Connect() and torn down on
  // Disconnect() / error.
  std::unique_ptr<binary::ClientTransport> transport_;
  std::unique_ptr<binary::ClientSecureChannel> secure_channel_;
  std::unique_ptr<binary::ClientConnection> connection_;
  std::unique_ptr<ClientChannel> channel_;
  std::unique_ptr<ClientProtocolSession> session_;

  bool is_connected_ = false;
  std::string endpoint_url_;

  // Lazily created on first CreateMonitoredItem.
  std::shared_ptr<ClientSubscription> default_subscription_;

  boost::signals2::signal<void(bool, const scada::Status&)>
      session_state_changed_;
};

}  // namespace opcua
