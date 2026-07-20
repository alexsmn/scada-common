#pragma once

// Client-side inverse adapters: present opcuapp's concrete ClientSession as the
// SCADA `core` services (scada::*Service), assembled into a ::DataServices the
// client consumes. Each method converts its arguments opcua<-core, calls the
// session, and converts the result core<-opcua -- the mirror of
// server_adapters.h.

#include "metrics/tracer.h"
#include "opcua_bridge/service_conversion.h"

#include "scada/data_services.h"
#include "scada/history_service.h"
#include "scada/history_update_service.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"

#include "opcua/client/client_session.h"

#include <memory>
#include <span>
#include <vector>

namespace scada::opcua_bridge {

class ClientSessionServiceAdapter : public scada::SessionService {
 public:
  explicit ClientSessionServiceAdapter(std::shared_ptr<opcua::ClientSession> s)
      : session_{std::move(s)} {}

  Awaitable<void> Connect(scada::SessionConnectParams params) override;
  // Forward to opcuapp's ClientSession::ConnectStatus so connect/activation
  // failures (e.g. Bad_WrongLoginCredentials) surface to the client. Without
  // this override the core SessionService default runs Connect() and
  // unconditionally reports Good, swallowing every connect failure.
  Awaitable<scada::Status> ConnectStatus(
      scada::SessionConnectParams params) override;
  Awaitable<void> Reconnect() override;
  Awaitable<void> Disconnect() override;

  bool IsConnected(base::TimeDelta* ping_delay = nullptr) const override {
    if (!ping_delay)
      return session_->IsConnected(nullptr);
    opcua::Duration opcua_ping;
    const bool connected = session_->IsConnected(&opcua_ping);
    *ping_delay = ToScada(opcua_ping);
    return connected;
  }
  scada::NodeId GetUserId() const override {
    return ToScada(session_->GetUserId());
  }
  bool HasPrivilege(scada::Privilege privilege) const override {
    return session_->HasPrivilege(ToOpcua(privilege));
  }
  std::string GetHostName() const override { return session_->GetHostName(); }
  bool IsScada() const override { return session_->IsScada(); }

  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override {
    return session_->SubscribeSessionStateChanged(
        [callback](bool connected, const opcua::Status& status) {
          callback(connected, ToScada(status));
        });
  }
  // The session debugger is an opaque diagnostic type that cannot be bridged.
  scada::SessionDebugger* GetSessionDebugger() override { return nullptr; }

 private:
  std::shared_ptr<opcua::ClientSession> session_;
};

class ClientViewServiceAdapter : public scada::ViewService {
 public:
  explicit ClientViewServiceAdapter(std::shared_ptr<opcua::ClientSession> s,
                                    Tracer& tracer = Tracer::None())
      : session_{std::move(s)}, tracer_{tracer} {}

  Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>> Browse(
      scada::ServiceContext context,
      std::vector<scada::BrowseDescription> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
  Tracer& tracer_;
};

class ClientAttributeServiceAdapter : public scada::AttributeService {
 public:
  explicit ClientAttributeServiceAdapter(
      std::shared_ptr<opcua::ClientSession> s,
      Tracer& tracer = Tracer::None())
      : session_{std::move(s)}, tracer_{tracer} {}

  Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> Read(
      scada::ServiceContext context,
      std::vector<scada::ReadValueId> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> Write(
      scada::ServiceContext context,
      std::vector<scada::WriteValue> inputs) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
  Tracer& tracer_;
};

class ClientMethodServiceAdapter : public scada::MethodService {
 public:
  explicit ClientMethodServiceAdapter(std::shared_ptr<opcua::ClientSession> s,
                                      Tracer& tracer = Tracer::None())
      : session_{std::move(s)}, tracer_{tracer} {}

  Awaitable<scada::Status> Call(scada::NodeId node_id,
                                scada::NodeId method_id,
                                std::vector<scada::Variant> arguments,
                                scada::ServiceContext context) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
  Tracer& tracer_;
};

class ClientNodeManagementServiceAdapter : public scada::NodeManagementService {
 public:
  explicit ClientNodeManagementServiceAdapter(
      std::shared_ptr<opcua::ClientSession> s,
      Tracer& tracer = Tracer::None())
      : session_{std::move(s)}, tracer_{tracer} {}

  Awaitable<scada::StatusOr<std::vector<scada::AddNodesResult>>> AddNodes(
      scada::ServiceContext context,
      std::vector<scada::AddNodesItem> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> DeleteNodes(
      scada::ServiceContext context,
      std::vector<scada::DeleteNodesItem> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> AddReferences(
      scada::ServiceContext context,
      std::vector<scada::AddReferencesItem> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> DeleteReferences(
      scada::ServiceContext context,
      std::vector<scada::DeleteReferencesItem> inputs) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
  Tracer& tracer_;
};

// Wraps an inner opcua MonitoredItemSubscription as the core interface.
class ClientMonitoredItemSubscriptionAdapter
    : public scada::MonitoredItemSubscription {
 public:
  explicit ClientMonitoredItemSubscriptionAdapter(
      std::unique_ptr<opcua::MonitoredItemSubscription> inner)
      : inner_{std::move(inner)} {}

  Awaitable<std::vector<scada::MonitoredItemCreateResult>> AddItems(
      std::vector<scada::MonitoredItemCreateRequest> requests) override;
  Awaitable<std::vector<scada::Status>> RemoveItems(
      std::span<const scada::MonitoredItemId> item_ids) override;
  Awaitable<scada::StatusOr<std::vector<scada::MonitoredItemNotification>>>
  ReadNext(std::size_t max_count) override;
  void Close(scada::Status status) override;

 private:
  std::unique_ptr<opcua::MonitoredItemSubscription> inner_;
};

class ClientMonitoredItemServiceAdapter : public scada::MonitoredItemService {
 public:
  explicit ClientMonitoredItemServiceAdapter(
      std::shared_ptr<opcua::ClientSession> s,
      Tracer& tracer = Tracer::None())
      : session_{std::move(s)}, tracer_{tracer} {}

  scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
  CreateSubscription(scada::ServiceContext context,
                     scada::MonitoredItemSubscriptionOptions options) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
  Tracer& tracer_;
};

// Presents a remote OPC UA historian (reached over the client session) as the
// core history read + update services. Implements both interfaces because the
// SCADA historian exposes both; the server-side router cross-casts a registered
// HistoryService to its update interface (mirrors RootNodeManager).
class ClientHistoryServiceAdapter : public scada::HistoryService,
                                    public scada::HistoryUpdateService {
 public:
  explicit ClientHistoryServiceAdapter(std::shared_ptr<opcua::ClientSession> s,
                                       Tracer& tracer = Tracer::None())
      : session_{std::move(s)}, tracer_{tracer} {}

  // scada::HistoryService
  Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override;
  Awaitable<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId node_id,
      base::Time from,
      base::Time to,
      scada::EventFilter filter) override;

  // scada::HistoryUpdateService
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> HistoryUpdateData(
      scada::ServiceContext context,
      scada::UpdateDataDetails details) override;
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> HistoryUpdateEvent(
      scada::ServiceContext context,
      scada::UpdateEventDetails details) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
  Tracer& tracer_;
};

// Assembles a ::DataServices backed by the given opcua client session.
// `tracer` (typically the core module's) makes every context-carrying call
// emit a CLIENT span and propagate its traceparent to the remote tier via the
// OPC UA request header.
::DataServices CreateClientDataServices(
    std::shared_ptr<opcua::ClientSession> session,
    Tracer& tracer = Tracer::None());

// Probes a peer's Server_ServiceLevel (i=2267) over a throwaway session, so
// probing a standby does not disturb the active session. Shared by the
// redundant inter-tier clients (remote historian / aggregating proxy / remote
// config) for ServiceLevel-aware failover. OPC UA Part 4 §6.6 Non-Transparent
// Redundancy, https://reference.opcfoundation.org/Core/Part4/v105/docs/6.6
[[nodiscard]] Awaitable<scada::StatusOr<scada::UInt8>> ProbeServiceLevel(
    AnyExecutor executor,
    transport::TransportFactory& transport_factory,
    std::string endpoint,
    scada::LocalizedText user_name,
    scada::LocalizedText password);

}  // namespace scada::opcua_bridge
