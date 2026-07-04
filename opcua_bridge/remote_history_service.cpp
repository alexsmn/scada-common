#include "opcua_bridge/remote_history_service.h"

#include "scada/data_value.h"
#include "scada/read_value_id.h"
#include "scada/service_context.h"
#include "scada/variant.h"

#include <utility>

namespace opcua_bridge {
namespace {

// OPC UA Part 5 §6.3.1: Server_ServiceLevel, the standard health indicator a
// non-transparent-redundancy client uses to pick the best server.
constexpr scada::NumericId kServerServiceLevelId = 2267;

// Reads Server_ServiceLevel over the given session.
Awaitable<scada::StatusOr<scada::UInt8>> ReadServiceLevelVia(
    std::shared_ptr<opcua::ClientSession> session) {
  ClientAttributeServiceAdapter attr{std::move(session)};
  auto inputs = std::make_shared<std::vector<scada::ReadValueId>>();
  inputs->push_back(scada::ReadValueId{.node_id = scada::NodeId{
                                           kServerServiceLevelId}});
  auto result = co_await attr.Read(scada::ServiceContext{}, std::move(inputs));
  if (!result.ok()) {
    co_return result.status();
  }
  if (result->empty() || !scada::IsGood(result->front().status_code)) {
    co_return scada::Status{scada::StatusCode::Bad};
  }
  scada::UInt8 level = 0;
  if (!result->front().value.get(level)) {
    co_return scada::Status{scada::StatusCode::Bad};
  }
  co_return level;
}

}  // namespace

RemoteHistoryService::RemoteHistoryService(
    AnyExecutor executor,
    transport::TransportFactory& transport_factory,
    RemoteHistoryServiceConfig config)
    : executor_{std::move(executor)},
      transport_factory_{transport_factory},
      config_{std::move(config)},
      session_{std::make_shared<opcua::ClientSession>(executor_,
                                                      transport_factory_)},
      adapter_{session_} {}

RemoteHistoryService::~RemoteHistoryService() = default;

Awaitable<scada::Status> RemoteHistoryService::Connect() {
  return ConnectTo(config_.endpoint_url);
}

Awaitable<scada::Status> RemoteHistoryService::ConnectTo(std::string endpoint) {
  auto status = co_await session_->ConnectStatus(
      opcua::SessionConnectParams{.connection_string = std::move(endpoint),
                                  .user_name = config_.user_name,
                                  .password = config_.password,
                                  .security = config_.security});
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

Awaitable<scada::StatusOr<scada::UInt8>>
RemoteHistoryService::ReadServiceLevel() {
  return ReadServiceLevelVia(session_);
}

Awaitable<scada::StatusOr<scada::UInt8>>
RemoteHistoryService::ProbeServiceLevel(std::string endpoint) {
  return opcua_bridge::ProbeServiceLevel(executor_, transport_factory_,
                                         std::move(endpoint), config_.user_name,
                                         config_.password);
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
RemoteHistoryService::HistoryUpdateData(scada::ServiceContext context,
                                        scada::UpdateDataDetails details) {
  return adapter_.HistoryUpdateData(std::move(context), std::move(details));
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
RemoteHistoryService::HistoryUpdateEvent(scada::ServiceContext context,
                                         scada::UpdateEventDetails details) {
  return adapter_.HistoryUpdateEvent(std::move(context), std::move(details));
}

}  // namespace opcua_bridge
