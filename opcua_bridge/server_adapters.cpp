#include "opcua_bridge/server_adapters.h"

#include "opcua_bridge/vector_conversion.h"

namespace opcua_bridge {

// --- AttributeService ---------------------------------------------------
opcua::Awaitable<opcua::StatusOr<std::vector<opcua::DataValue>>>
AttributeServiceAdapter::Read(
    opcua::ServiceContext context,
    std::shared_ptr<const std::vector<opcua::ReadValueId>> inputs) {
  auto scada_inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      ToScadaVector(*inputs));
  auto result = co_await inner_.Read(ToScada(context), std::move(scada_inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
AttributeServiceAdapter::Write(
    opcua::ServiceContext context,
    std::shared_ptr<const std::vector<opcua::WriteValue>> inputs) {
  auto scada_inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      ToScadaVector(*inputs));
  auto result = co_await inner_.Write(ToScada(context), std::move(scada_inputs));
  co_return ToOpcua(result);
}

// --- ViewService --------------------------------------------------------
opcua::Awaitable<opcua::StatusOr<std::vector<opcua::BrowseResult>>>
ViewServiceAdapter::Browse(
    opcua::ServiceContext context,
    std::vector<opcua::BrowseDescription> inputs) {
  auto result =
      co_await inner_.Browse(ToScada(context), ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<
    opcua::StatusOr<std::vector<opcua::BrowsePathResult>>>
ViewServiceAdapter::TranslateBrowsePaths(
    std::vector<opcua::BrowsePath> inputs) {
  auto result = co_await inner_.TranslateBrowsePaths(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

// --- MethodService ------------------------------------------------------
opcua::Awaitable<opcua::Status> MethodServiceAdapter::Call(
    opcua::NodeId node_id,
    opcua::NodeId method_id,
    std::vector<opcua::Variant> arguments,
    opcua::NodeId user_id) {
  auto status = co_await inner_.Call(ToScada(node_id), ToScada(method_id),
                                     ToScadaVector(arguments), ToScada(user_id));
  co_return ToOpcua(status);
}

// --- NodeManagementService ---------------------------------------------
opcua::Awaitable<
    opcua::StatusOr<std::vector<opcua::AddNodesResult>>>
NodeManagementServiceAdapter::AddNodes(
    std::vector<opcua::AddNodesItem> inputs) {
  auto result = co_await inner_.AddNodes(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
NodeManagementServiceAdapter::DeleteNodes(
    std::vector<opcua::DeleteNodesItem> inputs) {
  auto result = co_await inner_.DeleteNodes(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
NodeManagementServiceAdapter::AddReferences(
    std::vector<opcua::AddReferencesItem> inputs) {
  auto result = co_await inner_.AddReferences(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
NodeManagementServiceAdapter::DeleteReferences(
    std::vector<opcua::DeleteReferencesItem> inputs) {
  auto result = co_await inner_.DeleteReferences(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

// --- HistoryService -----------------------------------------------------
opcua::Awaitable<opcua::HistoryReadRawResult>
HistoryServiceAdapter::HistoryReadRaw(
    opcua::HistoryReadRawDetails details) {
  auto result = co_await inner_.HistoryReadRaw(ToScada(details));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::HistoryReadEventsResult>
HistoryServiceAdapter::HistoryReadEvents(opcua::NodeId node_id,
                                         opcua::base::Time from,
                                         opcua::base::Time to,
                                         opcua::EventFilter filter) {
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

opcua::Awaitable<std::vector<opcua::Status>>
MonitoredItemSubscriptionAdapter::RemoveItems(
    std::span<const opcua::scada::MonitoredItemId> item_ids) {
  // MonitoredItemId is std::uint32_t on both sides.
  auto results = co_await inner_->RemoveItems(item_ids);
  co_return ToOpcuaVector(results);
}

opcua::Awaitable<
    opcua::StatusOr<std::vector<opcua::scada::MonitoredItemNotification>>>
MonitoredItemSubscriptionAdapter::ReadNext(std::size_t max_count) {
  auto result = co_await inner_->ReadNext(max_count);
  co_return ToOpcua(result);
}

void MonitoredItemSubscriptionAdapter::Close(opcua::Status status) {
  inner_->Close(ToScada(status));
}

// --- MonitoredItemService ----------------------------------------------
opcua::StatusOr<std::unique_ptr<opcua::scada::MonitoredItemSubscription>>
MonitoredItemServiceAdapter::CreateSubscription(
    opcua::ServiceContext context,
    opcua::scada::MonitoredItemSubscriptionOptions options) {
  auto result =
      inner_.CreateSubscription(ToScada(context), ToScada(options));
  if (!result.ok())
    return ToOpcua(result.status());
  return std::unique_ptr<opcua::scada::MonitoredItemSubscription>{
      std::make_unique<MonitoredItemSubscriptionAdapter>(std::move(*result))};
}

// --- Authenticator ------------------------------------------------------
opcua::Awaitable<opcua::StatusOr<opcua::AuthenticationResult>>
AuthenticatorAdapter::Authenticate(opcua::LocalizedText user_name,
                                   opcua::LocalizedText password) {
  // LocalizedText is std::u16string on both sides.
  auto result = co_await inner_->Authenticate(user_name, password);
  if (!result.ok())
    co_return ToOpcua(result.status());
  co_return ToOpcua(*result);
}

}  // namespace opcua_bridge
