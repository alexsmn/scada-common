#include "opcua_bridge/server_adapters.h"

#include "opcua_bridge/vector_conversion.h"

namespace opcua_bridge {

// --- AttributeService ---------------------------------------------------
opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::DataValue>>>
AttributeServiceAdapter::Read(
    opcua::scada::ServiceContext context,
    std::shared_ptr<const std::vector<opcua::scada::ReadValueId>> inputs) {
  auto scada_inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      ToScadaVector(*inputs));
  auto result = co_await inner_.Read(ToScada(context), std::move(scada_inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::StatusCode>>>
AttributeServiceAdapter::Write(
    opcua::scada::ServiceContext context,
    std::shared_ptr<const std::vector<opcua::scada::WriteValue>> inputs) {
  auto scada_inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      ToScadaVector(*inputs));
  auto result = co_await inner_.Write(ToScada(context), std::move(scada_inputs));
  co_return ToOpcua(result);
}

// --- ViewService --------------------------------------------------------
opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::BrowseResult>>>
ViewServiceAdapter::Browse(
    opcua::scada::ServiceContext context,
    std::vector<opcua::scada::BrowseDescription> inputs) {
  auto result =
      co_await inner_.Browse(ToScada(context), ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<
    opcua::scada::StatusOr<std::vector<opcua::scada::BrowsePathResult>>>
ViewServiceAdapter::TranslateBrowsePaths(
    std::vector<opcua::scada::BrowsePath> inputs) {
  auto result = co_await inner_.TranslateBrowsePaths(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

// --- MethodService ------------------------------------------------------
opcua::Awaitable<opcua::scada::Status> MethodServiceAdapter::Call(
    opcua::scada::NodeId node_id,
    opcua::scada::NodeId method_id,
    std::vector<opcua::scada::Variant> arguments,
    opcua::scada::NodeId user_id) {
  auto status = co_await inner_.Call(ToScada(node_id), ToScada(method_id),
                                     ToScadaVector(arguments), ToScada(user_id));
  co_return ToOpcua(status);
}

// --- NodeManagementService ---------------------------------------------
opcua::Awaitable<
    opcua::scada::StatusOr<std::vector<opcua::scada::AddNodesResult>>>
NodeManagementServiceAdapter::AddNodes(
    std::vector<opcua::scada::AddNodesItem> inputs) {
  auto result = co_await inner_.AddNodes(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::StatusCode>>>
NodeManagementServiceAdapter::DeleteNodes(
    std::vector<opcua::scada::DeleteNodesItem> inputs) {
  auto result = co_await inner_.DeleteNodes(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::StatusCode>>>
NodeManagementServiceAdapter::AddReferences(
    std::vector<opcua::scada::AddReferencesItem> inputs) {
  auto result = co_await inner_.AddReferences(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::StatusCode>>>
NodeManagementServiceAdapter::DeleteReferences(
    std::vector<opcua::scada::DeleteReferencesItem> inputs) {
  auto result = co_await inner_.DeleteReferences(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

// --- HistoryService -----------------------------------------------------
opcua::Awaitable<opcua::scada::HistoryReadRawResult>
HistoryServiceAdapter::HistoryReadRaw(
    opcua::scada::HistoryReadRawDetails details) {
  auto result = co_await inner_.HistoryReadRaw(ToScada(details));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::scada::HistoryReadEventsResult>
HistoryServiceAdapter::HistoryReadEvents(opcua::scada::NodeId node_id,
                                         opcua::base::Time from,
                                         opcua::base::Time to,
                                         opcua::scada::EventFilter filter) {
  auto result = co_await inner_.HistoryReadEvents(
      ToScada(node_id), ToScada(from), ToScada(to), ToScada(filter));
  co_return ToOpcua(result);
}

// --- MonitoredItemSubscription -----------------------------------------
opcua::Awaitable<std::vector<opcua::scada::MonitoredItemCreateResult>>
MonitoredItemSubscriptionAdapter::AddItems(
    std::vector<opcua::scada::MonitoredItemCreateRequest> requests) {
  auto results = co_await inner_->AddItems(ToScadaVector(requests));
  co_return ToOpcuaVector(results);
}

opcua::Awaitable<std::vector<opcua::scada::Status>>
MonitoredItemSubscriptionAdapter::RemoveItems(
    std::span<const opcua::scada::MonitoredItemId> item_ids) {
  // MonitoredItemId is std::uint32_t on both sides.
  auto results = co_await inner_->RemoveItems(item_ids);
  co_return ToOpcuaVector(results);
}

opcua::Awaitable<
    opcua::scada::StatusOr<std::vector<opcua::scada::MonitoredItemNotification>>>
MonitoredItemSubscriptionAdapter::ReadNext(std::size_t max_count) {
  auto result = co_await inner_->ReadNext(max_count);
  co_return ToOpcua(result);
}

void MonitoredItemSubscriptionAdapter::Close(opcua::scada::Status status) {
  inner_->Close(ToScada(status));
}

// --- MonitoredItemService ----------------------------------------------
opcua::scada::StatusOr<std::unique_ptr<opcua::scada::MonitoredItemSubscription>>
MonitoredItemServiceAdapter::CreateSubscription(
    opcua::scada::ServiceContext context,
    opcua::scada::MonitoredItemSubscriptionOptions options) {
  auto result =
      inner_.CreateSubscription(ToScada(context), ToScada(options));
  if (!result.ok())
    return ToOpcua(result.status());
  return std::unique_ptr<opcua::scada::MonitoredItemSubscription>{
      std::make_unique<MonitoredItemSubscriptionAdapter>(std::move(*result))};
}

// --- Authenticator ------------------------------------------------------
opcua::Awaitable<opcua::scada::StatusOr<opcua::scada::AuthenticationResult>>
AuthenticatorAdapter::Authenticate(opcua::scada::LocalizedText user_name,
                                   opcua::scada::LocalizedText password) {
  // LocalizedText is std::u16string on both sides.
  auto result = co_await inner_->Authenticate(user_name, password);
  if (!result.ok())
    co_return ToOpcua(result.status());
  co_return ToOpcua(*result);
}

}  // namespace opcua_bridge
