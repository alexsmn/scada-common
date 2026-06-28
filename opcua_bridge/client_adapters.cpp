#include "opcua_bridge/client_adapters.h"

#include "opcua_bridge/vector_conversion.h"
#include "scada/read_value_id.h"
#include "scada/service_awaitable.h"
#include "scada/standard_node_ids.h"

namespace opcua_bridge {

// --- SessionService -----------------------------------------------------
Awaitable<void> ClientSessionServiceAdapter::Connect(
    scada::SessionConnectParams params) {
  co_await session_->Connect(ToOpcua(params));
}
Awaitable<scada::Status> ClientSessionServiceAdapter::ConnectStatus(
    scada::SessionConnectParams params) {
  co_return ToScada(co_await session_->ConnectStatus(ToOpcua(params)));
}
Awaitable<void> ClientSessionServiceAdapter::Reconnect() {
  co_await session_->Reconnect();
}
Awaitable<void> ClientSessionServiceAdapter::Disconnect() {
  co_await session_->Disconnect();
}

// --- ViewService --------------------------------------------------------
Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
ClientViewServiceAdapter::Browse(scada::ServiceContext context,
                                 std::vector<scada::BrowseDescription> inputs) {
  auto result =
      co_await session_->Browse(ToOpcua(context), ToOpcuaVector(inputs));
  co_return ToScada(result);
}

Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
ClientViewServiceAdapter::TranslateBrowsePaths(
    std::vector<scada::BrowsePath> inputs) {
  auto result = co_await session_->TranslateBrowsePaths(ToOpcuaVector(inputs));
  co_return ToScada(result);
}

// --- AttributeService ---------------------------------------------------
Awaitable<scada::StatusOr<std::vector<scada::DataValue>>>
ClientAttributeServiceAdapter::Read(
    scada::ServiceContext context,
    std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) {
  auto opcua_inputs = std::make_shared<const std::vector<opcua::ReadValueId>>(
      ToOpcuaVector(*inputs));
  auto result =
      co_await session_->Read(ToOpcua(context), std::move(opcua_inputs));
  co_return ToScada(result);
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientAttributeServiceAdapter::Write(
    scada::ServiceContext context,
    std::shared_ptr<const std::vector<scada::WriteValue>> inputs) {
  auto opcua_inputs = std::make_shared<const std::vector<opcua::WriteValue>>(
      ToOpcuaVector(*inputs));
  auto result =
      co_await session_->Write(ToOpcua(context), std::move(opcua_inputs));
  co_return ToScada(result);
}

// --- MethodService ------------------------------------------------------
Awaitable<scada::Status> ClientMethodServiceAdapter::Call(
    scada::NodeId node_id,
    scada::NodeId method_id,
    std::vector<scada::Variant> arguments,
    scada::NodeId user_id) {
  auto status =
      co_await session_->Call(ToOpcua(node_id), ToOpcua(method_id),
                              ToOpcuaVector(arguments), ToOpcua(user_id));
  co_return ToScada(status);
}

// --- NodeManagementService ---------------------------------------------
Awaitable<scada::StatusOr<std::vector<scada::AddNodesResult>>>
ClientNodeManagementServiceAdapter::AddNodes(
    std::vector<scada::AddNodesItem> inputs) {
  auto result = co_await session_->AddNodes(ToOpcuaVector(inputs));
  co_return ToScada(result);
}
Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientNodeManagementServiceAdapter::DeleteNodes(
    std::vector<scada::DeleteNodesItem> inputs) {
  auto result = co_await session_->DeleteNodes(ToOpcuaVector(inputs));
  co_return ToScada(result);
}
Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientNodeManagementServiceAdapter::AddReferences(
    std::vector<scada::AddReferencesItem> inputs) {
  auto result = co_await session_->AddReferences(ToOpcuaVector(inputs));
  co_return ToScada(result);
}
Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientNodeManagementServiceAdapter::DeleteReferences(
    std::vector<scada::DeleteReferencesItem> inputs) {
  auto result = co_await session_->DeleteReferences(ToOpcuaVector(inputs));
  co_return ToScada(result);
}

// --- MonitoredItemSubscription -----------------------------------------
Awaitable<std::vector<scada::MonitoredItemCreateResult>>
ClientMonitoredItemSubscriptionAdapter::AddItems(
    std::vector<scada::MonitoredItemCreateRequest> requests) {
  auto results = co_await inner_->AddItems(ToOpcuaVector(requests));
  co_return ToScadaVector(results);
}
Awaitable<std::vector<scada::Status>>
ClientMonitoredItemSubscriptionAdapter::RemoveItems(
    std::span<const scada::MonitoredItemId> item_ids) {
  // MonitoredItemId is std::uint32_t on both sides.
  auto results = co_await inner_->RemoveItems(item_ids);
  co_return ToScadaVector(results);
}
Awaitable<scada::StatusOr<std::vector<scada::MonitoredItemNotification>>>
ClientMonitoredItemSubscriptionAdapter::ReadNext(std::size_t max_count) {
  auto result = co_await inner_->ReadNext(max_count);
  co_return ToScada(result);
}
void ClientMonitoredItemSubscriptionAdapter::Close(scada::Status status) {
  inner_->Close(ToOpcua(status));
}

// --- MonitoredItemService ----------------------------------------------
scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
ClientMonitoredItemServiceAdapter::CreateSubscription(
    scada::ServiceContext context,
    scada::MonitoredItemSubscriptionOptions options) {
  auto result =
      session_->CreateSubscription(ToOpcua(context), ToOpcua(options));
  if (!result.ok())
    return ToScada(result.status());
  return std::unique_ptr<scada::MonitoredItemSubscription>{
      std::make_unique<ClientMonitoredItemSubscriptionAdapter>(
          std::move(*result))};
}

// --- HistoryService / HistoryUpdateService -----------------------------
Awaitable<scada::HistoryReadRawResult>
ClientHistoryServiceAdapter::HistoryReadRaw(
    scada::HistoryReadRawDetails details) {
  auto result = co_await session_->HistoryReadRaw(ToOpcua(details));
  if (!result.ok()) {
    co_return scada::HistoryReadRawResult{.status = ToScada(result.status())};
  }
  co_return ToScada(*result);
}

Awaitable<scada::HistoryReadEventsResult>
ClientHistoryServiceAdapter::HistoryReadEvents(scada::NodeId node_id,
                                               base::Time from,
                                               base::Time to,
                                               scada::EventFilter filter) {
  scada::HistoryReadEventsDetails details{.node_id = std::move(node_id),
                                          .from = from,
                                          .to = to,
                                          .filter = std::move(filter)};
  auto result = co_await session_->HistoryReadEvents(ToOpcua(details));
  if (!result.ok()) {
    co_return scada::HistoryReadEventsResult{.status =
                                                 ToScada(result.status())};
  }
  co_return ToScada(*result);
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientHistoryServiceAdapter::HistoryUpdateData(
    scada::UpdateDataDetails details) {
  auto result = co_await session_->HistoryUpdateData(ToOpcua(details));
  if (!result.ok()) {
    co_return ToScada(result.status());
  }
  if (result->status.bad()) {
    co_return ToScada(result->status);
  }
  co_return ToScadaVector(result->operation_results);
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientHistoryServiceAdapter::HistoryUpdateEvent(
    scada::UpdateEventDetails details) {
  auto result = co_await session_->HistoryUpdateEvent(ToOpcua(details));
  if (!result.ok()) {
    co_return ToScada(result.status());
  }
  if (result->status.bad()) {
    co_return ToScada(result->status);
  }
  co_return ToScadaVector(result->operation_results);
}

// --- factory ------------------------------------------------------------
::DataServices CreateClientDataServices(
    std::shared_ptr<opcua::ClientSession> session) {
  ::DataServices services;
  services.session_service_ =
      std::make_shared<ClientSessionServiceAdapter>(session);
  services.view_service_ = std::make_shared<ClientViewServiceAdapter>(session);
  services.attribute_service_ =
      std::make_shared<ClientAttributeServiceAdapter>(session);
  services.method_service_ =
      std::make_shared<ClientMethodServiceAdapter>(session);
  services.node_management_service_ =
      std::make_shared<ClientNodeManagementServiceAdapter>(session);
  services.monitored_item_service_ =
      std::make_shared<ClientMonitoredItemServiceAdapter>(session);
  services.history_service_ =
      std::make_shared<ClientHistoryServiceAdapter>(session);
  return services;
}

Awaitable<scada::StatusOr<scada::UInt8>> ProbeServiceLevel(
    AnyExecutor executor,
    transport::TransportFactory& transport_factory,
    std::string endpoint,
    scada::LocalizedText user_name,
    scada::LocalizedText password) {
  auto probe = std::make_shared<opcua::ClientSession>(std::move(executor),
                                                      transport_factory);
  auto services = CreateClientDataServices(probe);
  auto status = co_await services.session_service_->ConnectStatus(
      scada::SessionConnectParams{.connection_string = std::move(endpoint),
                                  .user_name = std::move(user_name),
                                  .password = std::move(password)});
  if (!status) {
    co_return status;
  }
  auto value = co_await scada::Read(
      *services.attribute_service_, scada::ServiceContext{},
      scada::ReadValueId{scada::NodeId{scada::id::Server_ServiceLevel}});
  co_await services.session_service_->Disconnect();
  if (!scada::IsGood(value.status_code)) {
    co_return scada::Status{value.status_code};
  }
  scada::UInt8 level = 0;
  if (!value.value.get(level)) {
    co_return scada::Status{scada::StatusCode::Bad};
  }
  co_return level;
}

}  // namespace opcua_bridge
