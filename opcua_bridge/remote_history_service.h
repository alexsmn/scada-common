#pragma once

#include "metrics/tracer.h"
// A core history service backed by an *external* OPC UA historian reached as an
// OPC UA client. Owns a ClientSession and its connection lifecycle; delegates
// reads/updates over the wire via ClientHistoryServiceAdapter. This is the
// server-side end of the separable historian tier -- see
// docs/historian-opcua-pluggable-seam.md §7 "Reaching an external historian".

#include "opcua_bridge/client_adapters.h"

#include "scada/history_service.h"
#include "scada/history_update_service.h"
#include "scada/session_service.h"  // scada::LocalizedText
#include "scada/status.h"

#include "opcua/client/client_session.h"

#include <memory>
#include <string>
#include <vector>

namespace transport {
class TransportFactory;
}

namespace opcua_bridge {

// Connection settings for an external OPC UA historian.
struct RemoteHistoryServiceConfig {
  std::string endpoint_url;        // e.g. "opc.tcp://historian-host:4840"
  scada::LocalizedText user_name;  // empty = anonymous
  scada::LocalizedText password;
  // Endpoint security negotiation for the historian session (default None).
  opcua::SessionSecuritySettings security;
};

// Presents an external OPC UA historian as the core history read + update
// services. Implements both interfaces so the server-side router can route
// HistoryRead and HistoryUpdate to the remote historian (mirrors how
// RootNodeManager cross-casts a registered HistoryService to its update side).
class RemoteHistoryService : public scada::HistoryService,
                             public scada::HistoryUpdateService {
 public:
  // `tracer` makes the historian-bound client calls emit CLIENT spans that
  // propagate their trace to the remote historian.
  RemoteHistoryService(AnyExecutor executor,
                       transport::TransportFactory& transport_factory,
                       RemoteHistoryServiceConfig config,
                       Tracer& tracer = Tracer::None());
  ~RemoteHistoryService() override;

  // Opens the session to the configured (primary) historian endpoint and
  // returns the connect status (connect/activation failures surface here).
  [[nodiscard]] Awaitable<scada::Status> Connect();
  // Connects to a specific endpoint using the configured credentials. Used by a
  // failover loop that drives the candidate endpoint list itself.
  [[nodiscard]] Awaitable<scada::Status> ConnectTo(std::string endpoint);
  // Cheap liveness keepalive: round-trips a HistoryRead of the standard
  // ServerStatus node. Returns a connectivity Bad (e.g. Bad_Disconnected /
  // Bad_Timeout) when the session is no longer usable, letting a caller detect
  // a silent mid-session drop that IsConnected() would not surface promptly.
  [[nodiscard]] Awaitable<scada::Status> Probe();
  [[nodiscard]] Awaitable<void> Disconnect();
  [[nodiscard]] bool IsConnected() const;

  // Reads the current session's advertised Server_ServiceLevel (OPC UA Part 4
  // §6.6). Used by a failover loop to prefer the highest-ServiceLevel server.
  [[nodiscard]] Awaitable<scada::StatusOr<scada::UInt8>> ReadServiceLevel();
  // Opens a *transient* session to `endpoint`, reads its ServiceLevel, and
  // disconnects — without disturbing the active session — so the loop can
  // compare candidates for ServiceLevel-aware preemption. Returns a
  // connectivity Bad if the endpoint is unreachable (e.g. a gated standby with
  // its listener closed).
  [[nodiscard]] Awaitable<scada::StatusOr<scada::UInt8>> ProbeServiceLevel(
      std::string endpoint);

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
  AnyExecutor executor_;
  transport::TransportFactory& transport_factory_;
  RemoteHistoryServiceConfig config_;
  // Declaration order is load-bearing: the session must outlive the adapter
  // that holds it.
  std::shared_ptr<opcua::ClientSession> session_;
  ClientHistoryServiceAdapter adapter_;
};

}  // namespace opcua_bridge
