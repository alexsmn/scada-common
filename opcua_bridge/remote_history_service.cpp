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
  auto status = co_await session_->ConnectStatus(
      opcua::SessionConnectParams{.connection_string = config_.endpoint_url,
                                  .user_name = config_.user_name,
                                  .password = config_.password});
  co_return ToScada(status);
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
