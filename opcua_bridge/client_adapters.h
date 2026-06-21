#pragma once

// Client-side inverse adapters: present opcuapp's ClientSession (which
// implements the opcua::scada::*Service interfaces) as the SCADA `core`
// services (scada::*Service), assembled into a ::DataServices the client
// consumes. Each method converts its arguments opcua<-core, calls the session,
// and converts the result core<-opcua — the mirror of server_adapters.h.

#include "opcua_bridge/service_conversion.h"

#include "scada/data_services.h"
#include "scada/history_service.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"

#include "opcua/client_session.h"

#include <memory>
#include <span>
#include <vector>

namespace opcua_bridge {

class ClientSessionServiceAdapter : public scada::SessionService {
 public:
  explicit ClientSessionServiceAdapter(std::shared_ptr<opcua::ClientSession> s)
      : session_{std::move(s)} {}

  Awaitable<void> Connect(scada::SessionConnectParams params) override;
  Awaitable<void> Reconnect() override;
  Awaitable<void> Disconnect() override;

  bool IsConnected(base::TimeDelta* ping_delay = nullptr) const override {
    if (!ping_delay)
      return session_->IsConnected(nullptr);
    opcua::base::TimeDelta opcua_ping;
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
        [callback](bool connected, const opcua::scada::Status& status) {
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
  explicit ClientViewServiceAdapter(std::shared_ptr<opcua::ClientSession> s)
      : session_{std::move(s)} {}

  Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>> Browse(
      scada::ServiceContext context,
      std::vector<scada::BrowseDescription> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
};

class ClientAttributeServiceAdapter : public scada::AttributeService {
 public:
  explicit ClientAttributeServiceAdapter(std::shared_ptr<opcua::ClientSession> s)
      : session_{std::move(s)} {}

  Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> Read(
      scada::ServiceContext context,
      std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> Write(
      scada::ServiceContext context,
      std::shared_ptr<const std::vector<scada::WriteValue>> inputs) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
};

class ClientMethodServiceAdapter : public scada::MethodService {
 public:
  explicit ClientMethodServiceAdapter(std::shared_ptr<opcua::ClientSession> s)
      : session_{std::move(s)} {}

  Awaitable<scada::Status> Call(scada::NodeId node_id,
                                scada::NodeId method_id,
                                std::vector<scada::Variant> arguments,
                                scada::NodeId user_id) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
};

class ClientNodeManagementServiceAdapter
    : public scada::NodeManagementService {
 public:
  explicit ClientNodeManagementServiceAdapter(
      std::shared_ptr<opcua::ClientSession> s)
      : session_{std::move(s)} {}

  Awaitable<scada::StatusOr<std::vector<scada::AddNodesResult>>> AddNodes(
      std::vector<scada::AddNodesItem> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> DeleteNodes(
      std::vector<scada::DeleteNodesItem> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> AddReferences(
      std::vector<scada::AddReferencesItem> inputs) override;
  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> DeleteReferences(
      std::vector<scada::DeleteReferencesItem> inputs) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
};

// Wraps an inner opcua MonitoredItemSubscription as the core interface.
class ClientMonitoredItemSubscriptionAdapter
    : public scada::MonitoredItemSubscription {
 public:
  explicit ClientMonitoredItemSubscriptionAdapter(
      std::unique_ptr<opcua::scada::MonitoredItemSubscription> inner)
      : inner_{std::move(inner)} {}

  Awaitable<std::vector<scada::MonitoredItemCreateResult>> AddItems(
      std::vector<scada::MonitoredItemCreateRequest> requests) override;
  Awaitable<std::vector<scada::Status>> RemoveItems(
      std::span<const scada::MonitoredItemId> item_ids) override;
  Awaitable<scada::StatusOr<std::vector<scada::MonitoredItemNotification>>>
  ReadNext(std::size_t max_count) override;
  void Close(scada::Status status) override;

 private:
  std::unique_ptr<opcua::scada::MonitoredItemSubscription> inner_;
};

class ClientMonitoredItemServiceAdapter : public scada::MonitoredItemService {
 public:
  explicit ClientMonitoredItemServiceAdapter(
      std::shared_ptr<opcua::ClientSession> s)
      : session_{std::move(s)} {}

  scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
  CreateSubscription(
      scada::ServiceContext context,
      scada::MonitoredItemSubscriptionOptions options) override;

 private:
  std::shared_ptr<opcua::ClientSession> session_;
};

// History is not provided by the OPC UA client session; this stub returns a
// bad status (matching the prior in-tree behaviour).
class UnsupportedHistoryService : public scada::HistoryService {
 public:
  Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails) override {
    co_return scada::HistoryReadRawResult{.status = scada::StatusCode::Bad};
  }
  Awaitable<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId,
      base::Time,
      base::Time,
      scada::EventFilter) override {
    co_return scada::HistoryReadEventsResult{.status = scada::StatusCode::Bad};
  }
};

// Assembles a ::DataServices backed by the given opcua client session.
::DataServices CreateClientDataServices(
    std::shared_ptr<opcua::ClientSession> session);

}  // namespace opcua_bridge
