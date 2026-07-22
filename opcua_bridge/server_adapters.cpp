#include "opcua_bridge/server_adapters.h"

#include "metrics/trace_attribute_util.h"
#include "opcua_bridge/vector_conversion.h"

#include "scada/event_util.h"

#include "opcua/events/event_filter.h"

#include <any>
#include <variant>

namespace scada::opcua_bridge {
namespace {

// Parses the EventFilter select-clause browse paths from a wire monitoring
// filter, falling back to the default BaseEventType fields when none is
// present.
std::vector<std::vector<std::string>> ParseWireEventFieldPaths(
    const std::optional<opcua::MonitoringFilter>& filter) {
  const auto* raw_filter =
      filter ? std::get_if<boost::json::value>(&*filter) : nullptr;
  if (!raw_filter)
    return opcua::NormalizeEventFieldPaths({});
  return opcua::ParseEventFilterFieldPaths(*raw_filter);
}

}  // namespace

// Starts a SERVER span continuing the caller's trace from the request-header
// traceparent (delivered in the opcua ServiceContext by the server runtime),
// and rewrites the context so downstream calls parent from this span.
namespace {

// Splits a "address:port" peer (IPv6 addresses bracketed) into the OTel
// server-span attributes `client.address` / `client.port`,
// https://opentelemetry.io/docs/specs/semconv/general/attributes/#client-attributes.
void SetPeerAttributes(TraceSpan& span, const std::string& peer) {
  if (peer.empty()) {
    return;
  }
  const auto colon = peer.rfind(':');
  if (colon == std::string::npos) {
    span.SetAttribute("client.address", peer);
    return;
  }
  std::string_view address{peer.data(), colon};
  if (address.size() >= 2 && address.front() == '[' && address.back() == ']') {
    address = address.substr(1, address.size() - 2);
  }
  span.SetAttribute("client.address", address);
  span.SetAttribute("client.port", std::string_view{peer}.substr(colon + 1));
}

TraceSpan StartServerSpan(Tracer& tracer,
                          std::string_view name,
                          opcua::ServiceContext& context) {
  TraceSpan span =
      tracer.StartSpan(name, TraceSpanKind::kServer, context.trace_id());
  SetPeerAttributes(span, context.peer());
  // OTel identity convention: `user.id` is the authenticated caller,
  // https://opentelemetry.io/docs/specs/semconv/registry/attributes/user/.
  // Omitted for an anonymous session (null user id).
  if (!context.user_id().is_null()) {
    span.SetAttribute("user.id", context.user_id().ToString());
  }
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

opcua::ServiceCallbacks ServerServiceAdapters::MakeCallbacks() {
  return {
      .read =
          [this](
              opcua::ServiceContext context,
              std::shared_ptr<const std::vector<opcua::ReadValueId>> inputs) {
            return attribute.Read(std::move(context), std::move(inputs));
          },
      .write =
          [this](opcua::ServiceContext context,
                 std::shared_ptr<const std::vector<opcua::WriteValue>> inputs) {
            return attribute.Write(std::move(context), std::move(inputs));
          },
      .browse =
          [this](opcua::ServiceContext context,
                 std::vector<opcua::BrowseDescription> inputs) {
            return view.Browse(std::move(context), std::move(inputs));
          },
      .translate_browse_paths =
          [this](std::vector<opcua::BrowsePath> inputs) {
            return view.TranslateBrowsePaths(std::move(inputs));
          },
      .call =
          [this](opcua::NodeId node_id, opcua::NodeId method_id,
                 std::vector<opcua::Variant> arguments,
                 opcua::ServiceContext context) {
            return method.Call(std::move(node_id), std::move(method_id),
                               std::move(arguments), std::move(context));
          },
      .history_read_raw =
          [this](opcua::HistoryReadRawDetails details) {
            return history.HistoryReadRaw(std::move(details));
          },
      .history_read_events =
          [this](opcua::NodeId node_id, opcua::DateTime from,
                 opcua::DateTime to, opcua::EventFilter filter) {
            return history.HistoryReadEvents(std::move(node_id), from, to,
                                             std::move(filter));
          },
      .history_update =
          [this](opcua::ServiceContext context,
                 opcua::UpdateDataDetails details) {
            return history_update.HistoryUpdateData(std::move(context),
                                                    std::move(details));
          },
      .history_update_event =
          [this](opcua::ServiceContext context,
                 opcua::UpdateEventDetails details) {
            return history_update.HistoryUpdateEvent(std::move(context),
                                                     std::move(details));
          },
      .add_nodes =
          [this](opcua::ServiceContext context,
                 std::vector<opcua::AddNodesItem> inputs) {
            return node_management.AddNodes(std::move(context),
                                            std::move(inputs));
          },
      .delete_nodes =
          [this](opcua::ServiceContext context,
                 std::vector<opcua::DeleteNodesItem> inputs) {
            return node_management.DeleteNodes(std::move(context),
                                               std::move(inputs));
          },
      .add_references =
          [this](opcua::ServiceContext context,
                 std::vector<opcua::AddReferencesItem> inputs) {
            return node_management.AddReferences(std::move(context),
                                                 std::move(inputs));
          },
      .delete_references =
          [this](opcua::ServiceContext context,
                 std::vector<opcua::DeleteReferencesItem> inputs) {
            return node_management.DeleteReferences(std::move(context),
                                                    std::move(inputs));
          },
      .create_subscription =
          [this](opcua::ServiceContext context,
                 opcua::MonitoredItemSubscriptionOptions options) {
            return monitored_item.CreateSubscription(std::move(context),
                                                     std::move(options));
          },
  };
}

// --- AttributeService ---------------------------------------------------
opcua::Awaitable<opcua::StatusOr<std::vector<opcua::DataValue>>>
AttributeServiceAdapter::Read(
    opcua::ServiceContext context,
    std::shared_ptr<const std::vector<opcua::ReadValueId>> inputs) {
  auto span = StartServerSpan(tracer_, "opcua.server/Read", context);
  SetBatchAttributes(span, *inputs, [](const opcua::ReadValueId& input) {
    return input.node_id.ToString();
  });
  auto result = co_await inner_.Read(ToScada(context), ToScadaVector(*inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
AttributeServiceAdapter::Write(
    opcua::ServiceContext context,
    std::shared_ptr<const std::vector<opcua::WriteValue>> inputs) {
  auto span = StartServerSpan(tracer_, "opcua.server/Write", context);
  auto result = co_await inner_.Write(ToScada(context), ToScadaVector(*inputs));
  co_return ToOpcua(result);
}

// --- ViewService --------------------------------------------------------
opcua::Awaitable<opcua::StatusOr<std::vector<opcua::BrowseResult>>>
ViewServiceAdapter::Browse(opcua::ServiceContext context,
                           std::vector<opcua::BrowseDescription> inputs) {
  auto span = StartServerSpan(tracer_, "opcua.server/Browse", context);
  SetBatchAttributes(span, inputs, [](const opcua::BrowseDescription& input) {
    return input.node_id.ToString();
  });
  auto result = co_await inner_.Browse(ToScada(context), ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::BrowsePathResult>>>
ViewServiceAdapter::TranslateBrowsePaths(
    std::vector<opcua::BrowsePath> inputs) {
  // The translate callback carries no ServiceContext (see tracing.md), so the
  // request-header traceparent cannot reach this seam yet: a root SERVER span.
  auto span = tracer_.StartSpan("opcua.server/TranslateBrowsePaths",
                                TraceSpanKind::kServer, {});
  SetBatchAttributes(span, inputs, [](const opcua::BrowsePath& input) {
    return input.node_id.ToString();
  });
  auto result = co_await inner_.TranslateBrowsePaths(ToScadaVector(inputs));
  co_return ToOpcua(result);
}

// --- MethodService ------------------------------------------------------
opcua::Awaitable<opcua::Status> MethodServiceAdapter::Call(
    opcua::NodeId node_id,
    opcua::NodeId method_id,
    std::vector<opcua::Variant> arguments,
    opcua::ServiceContext context) {
  // ToScada(const opcua::ServiceContext&) carries the caller's user id and its
  // user_rights bitmask, so the scada MethodService sees the real caller for
  // permission enforcement (e.g. the Call permission at the router).
  auto span = StartServerSpan(tracer_, "opcua.server/Call", context);
  span.SetAttribute("scada.object_node_id", node_id.ToString());
  span.SetAttribute("scada.method_node_id", method_id.ToString());
  auto status =
      co_await inner_.Call(ToScada(node_id), ToScada(method_id),
                           ToScadaVector(arguments), ToScada(context));
  co_return ToOpcua(status);
}

// --- NodeManagementService ---------------------------------------------
opcua::Awaitable<opcua::StatusOr<std::vector<opcua::AddNodesResult>>>
NodeManagementServiceAdapter::AddNodes(
    opcua::ServiceContext context,
    std::vector<opcua::AddNodesItem> inputs) {
  auto span = StartServerSpan(tracer_, "opcua.server/AddNodes", context);
  SetBatchAttributes(span, inputs, [](const opcua::AddNodesItem& input) {
    return input.requested_id.ToString();
  });
  auto result =
      co_await inner_.AddNodes(ToScada(context), ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
NodeManagementServiceAdapter::DeleteNodes(
    opcua::ServiceContext context,
    std::vector<opcua::DeleteNodesItem> inputs) {
  auto span = StartServerSpan(tracer_, "opcua.server/DeleteNodes", context);
  SetBatchAttributes(span, inputs, [](const opcua::DeleteNodesItem& input) {
    return input.node_id.ToString();
  });
  auto result =
      co_await inner_.DeleteNodes(ToScada(context), ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
NodeManagementServiceAdapter::AddReferences(
    opcua::ServiceContext context,
    std::vector<opcua::AddReferencesItem> inputs) {
  auto span = StartServerSpan(tracer_, "opcua.server/AddReferences", context);
  SetBatchAttributes(span, inputs, [](const opcua::AddReferencesItem& input) {
    return input.source_node_id.ToString();
  });
  auto result =
      co_await inner_.AddReferences(ToScada(context), ToScadaVector(inputs));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
NodeManagementServiceAdapter::DeleteReferences(
    opcua::ServiceContext context,
    std::vector<opcua::DeleteReferencesItem> inputs) {
  auto span =
      StartServerSpan(tracer_, "opcua.server/DeleteReferences", context);
  SetBatchAttributes(span, inputs,
                     [](const opcua::DeleteReferencesItem& input) {
                       return input.source_node_id.ToString();
                     });
  auto result =
      co_await inner_.DeleteReferences(ToScada(context), ToScadaVector(inputs));
  co_return ToOpcua(result);
}

// --- HistoryService -----------------------------------------------------
opcua::Awaitable<opcua::HistoryReadRawResult>
HistoryServiceAdapter::HistoryReadRaw(opcua::HistoryReadRawDetails details) {
  // HistoryService carries no ServiceContext downstream (see tracing.md);
  // this SERVER span still anchors the historian-side work in the caller's
  // trace when the request header carried a traceparent -- which opcuapp
  // cannot deliver here yet, so today it is a root span.
  auto span = tracer_.StartSpan("opcua.server/HistoryReadRaw",
                                TraceSpanKind::kServer, {});
  span.SetAttribute("scada.node_id", details.node_id.ToString());
  auto result = co_await inner_.HistoryReadRaw(ToScada(details));
  co_return ToOpcua(result);
}

opcua::Awaitable<opcua::HistoryReadEventsResult>
HistoryServiceAdapter::HistoryReadEvents(opcua::NodeId node_id,
                                         opcua::DateTime from,
                                         opcua::DateTime to,
                                         opcua::EventFilter filter) {
  auto span = tracer_.StartSpan("opcua.server/HistoryReadEvents",
                                TraceSpanKind::kServer, {});
  span.SetAttribute("scada.node_id", node_id.ToString());
  auto result = co_await inner_.HistoryReadEvents(
      ToScada(node_id), ToScada(from), ToScada(to), ToScada(filter));
  co_return ToOpcua(result);
}

// --- HistoryUpdateService ----------------------------------------------
opcua::Awaitable<opcua::HistoryUpdateResult>
HistoryUpdateServiceAdapter::HistoryUpdateData(
    opcua::ServiceContext context,
    opcua::UpdateDataDetails details) {
  auto span =
      StartServerSpan(tracer_, "opcua.server/HistoryUpdateData", context);
  span.SetAttribute("scada.node_id", details.node_id.ToString());
  span.SetAttribute("scada.input_count", std::to_string(details.values.size()));
  // The core service returns a per-value StatusCode vector or an
  // operation-level failure; map both onto the wire HistoryUpdateResult.
  auto result =
      co_await inner_.HistoryUpdateData(ToScada(context), ToScada(details));
  if (!result.ok()) {
    co_return opcua::HistoryUpdateResult{.status = ToOpcua(result.status())};
  }
  co_return opcua::HistoryUpdateResult{.operation_results =
                                           ToOpcuaVector(*result)};
}

opcua::Awaitable<opcua::HistoryUpdateResult>
HistoryUpdateServiceAdapter::HistoryUpdateEvent(
    opcua::ServiceContext context,
    opcua::UpdateEventDetails details) {
  auto span =
      StartServerSpan(tracer_, "opcua.server/HistoryUpdateEvent", context);
  span.SetAttribute("scada.node_id", details.node_id.ToString());
  auto result =
      co_await inner_.HistoryUpdateEvent(ToScada(context), ToScada(details));
  if (!result.ok()) {
    co_return opcua::HistoryUpdateResult{.status = ToOpcua(result.status())};
  }
  co_return opcua::HistoryUpdateResult{.operation_results =
                                           ToOpcuaVector(*result)};
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
    if (!event_item_handle_.has_value() &&
        request.item_to_monitor.attribute_id ==
            opcua::AttributeId::EventNotifier) {
      event_item_handle_ = request.requested_parameters.client_handle;
    }
  }
  auto results = co_await inner_->AddItems(ToScadaVector(requests));
  co_return ToOpcuaVector(results);
}

opcua::ItemNotification MonitoredItemSubscriptionAdapter::ToItemNotification(
    const scada::MonitoredItemNotification& notification) const {
  return std::visit(
      [this](const auto& x) -> opcua::ItemNotification {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, scada::DataChangeNotification>) {
          return opcua::MonitoredItemNotification{
              .client_handle = x.client_handle, .value = ToOpcua(x.value)};
        } else if constexpr (std::is_same_v<T, scada::EventNotification>) {
          // Project the core event onto this item's EventFilter select clauses,
          // producing a standard wire EventFieldList. The core event payload is
          // a std::any carrying a scada::Event; convert it to opcua::Event,
          // then project by field path.
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
          } else if (x.event.has_value()) {
            // Non-system SCADA events (GeneralModelChangeEventType,
            // SemanticChangeEventType) have no OPC UA select-clause projection;
            // send their full per-type DisassembleEvent layout as the
            // EventFieldList, which the SCADA client reassembles via
            // AssembleEvent (symmetric SCADA-to-SCADA transport). OPC UA Part 4
            // §7.22.3 EventFilter / EventFieldList,
            // https://reference.opcfoundation.org/Core/Part4/v105/docs/7.22 ;
            // Part 3 §9.32.6 GeneralModelChangeEventType,
            // https://reference.opcfoundation.org/Core/Part3/v105/docs/9.32 .
            event_fields = ToOpcuaVector(scada::DisassembleEvent(x.event));
          } else {
            event_fields = opcua::ProjectEventFields(field_paths, std::any{});
          }
          return opcua::EventFieldList{.client_handle = x.client_handle,
                                       .event_fields = std::move(event_fields)};
        } else if constexpr (std::is_same_v<T, scada::OverflowNotification>) {
          if (event_item_handle_.has_value()) {
            // Event subscriptions surface the overflow as the standard
            // EventQueueOverflowEventType notification on the event item's
            // handle; it deliberately bypasses where clauses (OPC UA Part 4
            // §7.22,
            // https://reference.opcfoundation.org/Core/Part4/v105/docs/7.22).
            // The bridge has no event-id generator, so the EventId is
            // synthesized from the wall clock plus a process-wide counter —
            // unique enough for the "never null" rule, but not resolvable by
            // the Acknowledge method (overflow events are not ackable).
            static std::atomic<opcua::UInt64> counter{0};
            opcua::Event overflow_event;
            overflow_event.event_type_id =
                opcua::NodeId{opcua::id::EventQueueOverflowEventType};
            overflow_event.event_id =
                (static_cast<opcua::UInt64>(
                     opcua::DateTime::Now().ToInternalValue())
                 << 12) |
                (counter.fetch_add(1, std::memory_order_relaxed) & 0xfff);
            overflow_event.time = opcua::DateTime::Now();
            overflow_event.receive_time = overflow_event.time;
            overflow_event.severity = opcua::kSeverityCritical;
            overflow_event.source_node_id = opcua::NodeId{opcua::id::Server};
            overflow_event.source_name = "Server";
            overflow_event.message =
                u"Event queue overflowed; event notifications were lost";

            std::vector<std::vector<std::string>> field_paths;
            if (const auto it =
                    field_paths_by_handle_.find(*event_item_handle_);
                it != field_paths_by_handle_.end()) {
              field_paths = it->second;
            } else {
              field_paths = opcua::NormalizeEventFieldPaths({});
            }
            return opcua::EventFieldList{
                .client_handle = *event_item_handle_,
                .event_fields = opcua::ProjectEventFields(
                    field_paths, std::any{std::move(overflow_event)})};
          }
          // Overflow has no client_handle; surface as a status-only data-change
          // notification with handle 0 so the dropped-notifications signal
          // survives by status.
          opcua::DataValue value;
          value.status_code = ToOpcua(x.status).code();
          return opcua::MonitoredItemNotification{.client_handle = 0,
                                                  .value = value};
        } else {
          // ItemStatusNotification: no value or event payload. Surface as a
          // status-only data-change notification so the correlation/status
          // signal still reaches the consumer.
          opcua::DataValue value;
          value.status_code = ToOpcua(x.status).code();
          return opcua::MonitoredItemNotification{
              .client_handle = x.client_handle, .value = value};
        }
      },
      notification);
}

opcua::Awaitable<std::vector<opcua::Status>>
MonitoredItemSubscriptionAdapter::RemoveItems(
    std::span<const opcua::MonitoredItemId> item_ids) {
  // MonitoredItemId is std::uint32_t on both sides.
  auto results = co_await inner_->RemoveItems(item_ids);
  co_return ToOpcuaVector(results);
}

opcua::Awaitable<opcua::StatusOr<std::vector<opcua::ItemNotification>>>
MonitoredItemSubscriptionAdapter::ReadNext(std::size_t max_count) {
  auto result = co_await inner_->ReadNext(max_count);
  if (!result.ok())
    co_return ToOpcua(result.status());
  std::vector<opcua::ItemNotification> notifications;
  notifications.reserve(result->size());
  for (const auto& notification : *result)
    notifications.push_back(ToItemNotification(notification));
  co_return notifications;
}

void MonitoredItemSubscriptionAdapter::Close(opcua::Status status) {
  inner_->Close(ToScada(status));
}

// --- MonitoredItemService ----------------------------------------------
opcua::StatusOr<std::unique_ptr<opcua::MonitoredItemSubscription>>
MonitoredItemServiceAdapter::CreateSubscription(
    opcua::ServiceContext context,
    opcua::MonitoredItemSubscriptionOptions options) {
  auto span =
      StartServerSpan(tracer_, "opcua.server/CreateSubscription", context);
  auto result = inner_.CreateSubscription(ToScada(context), ToScada(options));
  if (!result.ok())
    return ToOpcua(result.status());
  return std::unique_ptr<opcua::MonitoredItemSubscription>{
      std::make_unique<MonitoredItemSubscriptionAdapter>(std::move(*result))};
}

// --- Authenticator ------------------------------------------------------
opcua::Awaitable<opcua::StatusOr<opcua::AuthenticationResult>>
AuthenticatorAdapter::Authenticate(opcua::LocalizedText user_name,
                                   opcua::LocalizedText password) {
  auto result =
      co_await inner_->Authenticate(ToScada(user_name), ToScada(password));
  if (!result.ok())
    co_return ToOpcua(result.status());
  co_return ToOpcua(*result);
}

}  // namespace scada::opcua_bridge
