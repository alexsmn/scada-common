#include "opcua_bridge/server_adapters.h"

#include "opcua_bridge/vector_conversion.h"

#include "opcua/endpoint_core.h"

#include <any>
#include <variant>

namespace opcua_bridge {
namespace {

// Parses the EventFilter select-clause browse paths from a wire monitoring
// filter, falling back to the default BaseEventType fields when none is present.
std::vector<std::vector<std::string>> ParseWireEventFieldPaths(
    const std::optional<opcua::MonitoringFilter>& filter) {
  const auto* raw_filter =
      filter ? std::get_if<boost::json::value>(&*filter) : nullptr;
  if (!raw_filter)
    return opcua::NormalizeEventFieldPaths({});
  return opcua::ParseEventFilterFieldPaths(*raw_filter);
}

}  // namespace

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
opcua::Awaitable<std::vector<opcua::MonitoredItemCreateResult>>
MonitoredItemSubscriptionAdapter::AddItems(
    std::vector<opcua::MonitoredItemCreateRequest> requests) {
  // Remember the EventFilter select-clause paths per client_handle so ReadNext
  // can project core events onto them into a standard wire EventFieldList.
  for (const auto& request : requests) {
    field_paths_by_handle_[request.requested_parameters.client_handle] =
        ParseWireEventFieldPaths(request.requested_parameters.filter);
  }
  auto results = co_await inner_->AddItems(ToScadaVector(requests));
  co_return ToOpcuaVector(results);
}

opcua::scada::ItemNotification
MonitoredItemSubscriptionAdapter::ToItemNotification(
    const scada::MonitoredItemNotification& notification) const {
  return std::visit(
      [this](const auto& x) -> opcua::scada::ItemNotification {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, scada::DataChangeNotification>) {
          return opcua::scada::MonitoredItemNotification{
              .client_handle = x.client_handle, .value = ToOpcua(x.value)};
        } else if constexpr (std::is_same_v<T, scada::EventNotification>) {
          // Project the core event onto this item's EventFilter select clauses,
          // producing a standard wire EventFieldList. The core event payload is
          // a std::any carrying a scada::Event; convert it to opcua::Event, then
          // project by field path.
          std::vector<std::vector<std::string>> field_paths;
          if (const auto it = field_paths_by_handle_.find(x.client_handle);
              it != field_paths_by_handle_.end()) {
            field_paths = it->second;
          } else {
            field_paths = opcua::NormalizeEventFieldPaths({});
          }
          std::vector<opcua::Variant> event_fields;
          if (const auto* scada_event = std::any_cast<scada::Event>(&x.event)) {
            event_fields = opcua::ProjectEventFields(
                field_paths, std::any{ToOpcua(*scada_event)});
          } else {
            event_fields =
                opcua::ProjectEventFields(field_paths, std::any{});
          }
          return opcua::scada::EventFieldList{
              .client_handle = x.client_handle,
              .event_fields = std::move(event_fields)};
        } else if constexpr (std::is_same_v<T, scada::OverflowNotification>) {
          // Overflow has no client_handle; surface as a status-only data-change
          // notification with handle 0 so the dropped-notifications signal
          // survives by status.
          opcua::DataValue value;
          value.status_code = ToOpcua(x.status).code();
          return opcua::scada::MonitoredItemNotification{.client_handle = 0,
                                                         .value = value};
        } else {
          // ItemStatusNotification: no value or event payload. Surface as a
          // status-only data-change notification so the correlation/status
          // signal still reaches the consumer.
          opcua::DataValue value;
          value.status_code = ToOpcua(x.status).code();
          return opcua::scada::MonitoredItemNotification{
              .client_handle = x.client_handle, .value = value};
        }
      },
      notification);
}

opcua::Awaitable<std::vector<opcua::Status>>
MonitoredItemSubscriptionAdapter::RemoveItems(
    std::span<const opcua::scada::MonitoredItemId> item_ids) {
  // MonitoredItemId is std::uint32_t on both sides.
  auto results = co_await inner_->RemoveItems(item_ids);
  co_return ToOpcuaVector(results);
}

opcua::Awaitable<
    opcua::StatusOr<std::vector<opcua::scada::ItemNotification>>>
MonitoredItemSubscriptionAdapter::ReadNext(std::size_t max_count) {
  auto result = co_await inner_->ReadNext(max_count);
  if (!result.ok())
    co_return ToOpcua(result.status());
  std::vector<opcua::scada::ItemNotification> notifications;
  notifications.reserve(result->size());
  for (const auto& notification : *result)
    notifications.push_back(ToItemNotification(notification));
  co_return notifications;
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
