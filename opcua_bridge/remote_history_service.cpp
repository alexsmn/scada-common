#include "opcua_bridge/remote_history_service.h"

#include <utility>

namespace opcua_bridge {

RemoteHistoryService::RemoteHistoryService(
    AnyExecutor executor,
    transport::TransportFactory& transport_factory,
    RemoteHistoryServiceConfig config)
    : config_{std::move(config)},
      session_{std::make_shared<opcua::ClientSession>(std::move(executor),
                                                      transport_factory)},
      adapter_{session_} {}

RemoteHistoryService::~RemoteHistoryService() = default;

Awaitable<scada::Status> RemoteHistoryService::Connect() {
  return ConnectTo(config_.endpoint_url);
}

Awaitable<scada::Status> RemoteHistoryService::ConnectTo(std::string endpoint) {
  auto status = co_await session_->ConnectStatus(
      opcua::SessionConnectParams{.connection_string = std::move(endpoint),
                                  .user_name = config_.user_name,
                                  .password = config_.password});
  co_return ToScada(status);
}

Awaitable<scada::Status> RemoteHistoryService::Probe() {
  // Liveness keepalive: round-trip a HistoryRead of the standard ServerStatus
  // CurrentTime node (i=2258). A live session returns Good/empty or a
  // non-connectivity Bad; a dead transport returns Bad_Disconnected/Bad_Timeout.
  auto result = co_await adapter_.HistoryReadRaw(
      scada::HistoryReadRawDetails{.node_id = scada::NodeId{2258}});
  co_return result.status;
}

Awaitable<void> RemoteHistoryService::Disconnect() {
  if (session_->IsConnected()) {
    co_await session_->Disconnect();
  }
}

bool RemoteHistoryService::IsConnected() const {
  return session_->IsConnected(nullptr);
}

Awaitable<scada::HistoryReadRawResult> RemoteHistoryService::HistoryReadRaw(
    scada::HistoryReadRawDetails details) {
  return adapter_.HistoryReadRaw(std::move(details));
}

Awaitable<scada::HistoryReadEventsResult>
RemoteHistoryService::HistoryReadEvents(scada::NodeId node_id,
                                        base::Time from,
                                        base::Time to,
                                        scada::EventFilter filter) {
  return adapter_.HistoryReadEvents(std::move(node_id), from, to,
                                    std::move(filter));
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
RemoteHistoryService::HistoryUpdateData(scada::UpdateDataDetails details) {
  return adapter_.HistoryUpdateData(std::move(details));
}

}  // namespace opcua_bridge
