#include "opcua_bridge/client_adapters.h"

#include "metrics/trace_attribute_util.h"
#include "opcua_bridge/vector_conversion.h"
#include "scada/read_value_id.h"
#include "scada/standard_node_ids.h"

namespace opcua_bridge {

namespace {

// Starts a CLIENT span parented from the context's trace and rewrites the
// context so the span's traceparent rides `ToOpcua(context)` into the OPC UA
// request header (see ServiceRequestHeader::trace_parent in opcuapp).
TraceSpan StartClientSpan(Tracer& tracer,
                          std::string_view name,
                          scada::ServiceContext& context) {
  TraceSpan span =
      tracer.StartSpan(name, TraceSpanKind::kClient, context.trace_id());
  if (std::string trace_parent = span.traceparent(); !trace_parent.empty()) {
    context = context.with_trace_id(trace_parent);
  }
  return span;
}

// Annotates a span with a batched request's size and (capped) node ids;
// `node_id_of` projects one input to its node id string.
template <class Items, class NodeIdOf>
void SetBatchAttributes(TraceSpan& span,
                        const Items& items,
                        NodeIdOf&& node_id_of) {
  span.SetAttribute("scada.input_count", std::to_string(std::size(items)));
  span.SetAttribute("scada.node_ids",
                    metrics::JoinForAttribute(items, node_id_of));
}

}  // namespace

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
  auto span = StartClientSpan(tracer_, "opcua.client/Browse", context);
  SetBatchAttributes(span, inputs, [](const scada::BrowseDescription& input) {
    return input.node_id.ToString();
  });
  auto result =
      co_await session_->Browse(ToOpcua(context), ToOpcuaVector(inputs));
  co_return ToScada(result);
}

Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
ClientViewServiceAdapter::TranslateBrowsePaths(
    std::vector<scada::BrowsePath> inputs) {
  // No ServiceContext on this service (see tracing.md): a root CLIENT span
  // whose traceparent still links the remote side.
  auto span = tracer_.StartSpan("opcua.client/TranslateBrowsePaths",
                                TraceSpanKind::kClient, {});
  SetBatchAttributes(span, inputs, [](const scada::BrowsePath& input) {
    return input.node_id.ToString();
  });
  auto result = co_await session_->TranslateBrowsePaths(ToOpcuaVector(inputs),
                                                        span.traceparent());
  co_return ToScada(result);
}

// --- AttributeService ---------------------------------------------------
Awaitable<scada::StatusOr<std::vector<scada::DataValue>>>
ClientAttributeServiceAdapter::Read(scada::ServiceContext context,
                                    std::vector<scada::ReadValueId> inputs) {
  auto span = StartClientSpan(tracer_, "opcua.client/Read", context);
  SetBatchAttributes(span, inputs, [](const scada::ReadValueId& input) {
    return input.node_id.ToString();
  });
  auto opcua_inputs = std::make_shared<const std::vector<opcua::ReadValueId>>(
      ToOpcuaVector(inputs));
  auto result =
      co_await session_->Read(ToOpcua(context), std::move(opcua_inputs));
  co_return ToScada(result);
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientAttributeServiceAdapter::Write(scada::ServiceContext context,
                                     std::vector<scada::WriteValue> inputs) {
  auto span = StartClientSpan(tracer_, "opcua.client/Write", context);
  SetBatchAttributes(span, inputs, [](const scada::WriteValue& input) {
    return input.node_id.ToString();
  });
  auto opcua_inputs = std::make_shared<const std::vector<opcua::WriteValue>>(
      ToOpcuaVector(inputs));
  auto result =
      co_await session_->Write(ToOpcua(context), std::move(opcua_inputs));
  co_return ToScada(result);
}

// --- MethodService ------------------------------------------------------
Awaitable<scada::Status> ClientMethodServiceAdapter::Call(
    scada::NodeId node_id,
    scada::NodeId method_id,
    std::vector<scada::Variant> arguments,
    scada::ServiceContext context) {
  auto span = StartClientSpan(tracer_, "opcua.client/Call", context);
  span.SetAttribute("scada.object_node_id", node_id.ToString());
  span.SetAttribute("scada.method_node_id", method_id.ToString());
  // The wire client session's Call still takes only a user id (the client does
  // not carry a rights bitmask to the server); extract it from the context.
  // The traceparent travels as an explicit argument instead.
  auto status = co_await session_->Call(
      ToOpcua(node_id), ToOpcua(method_id), ToOpcuaVector(arguments),
      ToOpcua(context.user_id()), span.traceparent());
  co_return ToScada(status);
}

// --- NodeManagementService ---------------------------------------------
Awaitable<scada::StatusOr<std::vector<scada::AddNodesResult>>>
ClientNodeManagementServiceAdapter::AddNodes(
    scada::ServiceContext context,
    std::vector<scada::AddNodesItem> inputs) {
  auto span = StartClientSpan(tracer_, "opcua.client/AddNodes", context);
  SetBatchAttributes(span, inputs, [](const scada::AddNodesItem& input) {
    return input.requested_id.ToString();
  });
  auto result =
      co_await session_->AddNodes(ToOpcuaVector(inputs), span.traceparent());
  co_return ToScada(result);
}
Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientNodeManagementServiceAdapter::DeleteNodes(
    scada::ServiceContext context,
    std::vector<scada::DeleteNodesItem> inputs) {
  auto span = StartClientSpan(tracer_, "opcua.client/DeleteNodes", context);
  SetBatchAttributes(span, inputs, [](const scada::DeleteNodesItem& input) {
    return input.node_id.ToString();
  });
  auto result =
      co_await session_->DeleteNodes(ToOpcuaVector(inputs), span.traceparent());
  co_return ToScada(result);
}
Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientNodeManagementServiceAdapter::AddReferences(
    scada::ServiceContext context,
    std::vector<scada::AddReferencesItem> inputs) {
  auto span = StartClientSpan(tracer_, "opcua.client/AddReferences", context);
  SetBatchAttributes(span, inputs, [](const scada::AddReferencesItem& input) {
    return input.source_node_id.ToString();
  });
  auto result = co_await session_->AddReferences(ToOpcuaVector(inputs),
                                                 span.traceparent());
  co_return ToScada(result);
}
Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientNodeManagementServiceAdapter::DeleteReferences(
    scada::ServiceContext context,
    std::vector<scada::DeleteReferencesItem> inputs) {
  auto span =
      StartClientSpan(tracer_, "opcua.client/DeleteReferences", context);
  SetBatchAttributes(span, inputs,
                     [](const scada::DeleteReferencesItem& input) {
                       return input.source_node_id.ToString();
                     });
  auto result = co_await session_->DeleteReferences(ToOpcuaVector(inputs),
                                                    span.traceparent());
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
  auto span =
      StartClientSpan(tracer_, "opcua.client/CreateSubscription", context);
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
  // HistoryService carries no ServiceContext (see tracing.md), so this CLIENT
  // span is a new root; its traceparent still links the historian-side spans.
  auto span = tracer_.StartSpan("opcua.client/HistoryReadRaw",
                                TraceSpanKind::kClient, {});
  span.SetAttribute("scada.node_id", details.node_id.ToString());
  auto result =
      co_await session_->HistoryReadRaw(ToOpcua(details), span.traceparent());
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
  auto span = tracer_.StartSpan("opcua.client/HistoryReadEvents",
                                TraceSpanKind::kClient, {});
  span.SetAttribute("scada.node_id", details.node_id.ToString());
  auto result = co_await session_->HistoryReadEvents(ToOpcua(details),
                                                     span.traceparent());
  if (!result.ok()) {
    co_return scada::HistoryReadEventsResult{.status =
                                                 ToScada(result.status())};
  }
  co_return ToScada(*result);
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
ClientHistoryServiceAdapter::HistoryUpdateData(
    scada::ServiceContext context,
    scada::UpdateDataDetails details) {
  auto span =
      StartClientSpan(tracer_, "opcua.client/HistoryUpdateData", context);
  auto result = co_await session_->HistoryUpdateData(ToOpcua(details),
                                                     span.traceparent());
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
    scada::ServiceContext context,
    scada::UpdateEventDetails details) {
  auto span =
      StartClientSpan(tracer_, "opcua.client/HistoryUpdateEvent", context);
  auto result = co_await session_->HistoryUpdateEvent(ToOpcua(details),
                                                      span.traceparent());
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
    std::shared_ptr<opcua::ClientSession> session,
    Tracer& tracer) {
  ::DataServices services;
  services.session_service_ =
      std::make_shared<ClientSessionServiceAdapter>(session);
  services.view_service_ =
      std::make_shared<ClientViewServiceAdapter>(session, tracer);
  services.attribute_service_ =
      std::make_shared<ClientAttributeServiceAdapter>(session, tracer);
  services.method_service_ =
      std::make_shared<ClientMethodServiceAdapter>(session, tracer);
  services.node_management_service_ =
      std::make_shared<ClientNodeManagementServiceAdapter>(session, tracer);
  services.monitored_item_service_ =
      std::make_shared<ClientMonitoredItemServiceAdapter>(session, tracer);
  services.history_service_ =
      std::make_shared<ClientHistoryServiceAdapter>(session, tracer);
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
