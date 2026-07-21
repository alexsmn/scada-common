#include "opcua_bridge/service_conversion.h"

#include "opcua_bridge/vector_conversion.h"

#include "scada/event_util.h"

#include "opcua/events/event_filter.h"
#include "opcua/events/event_util.h"

#include <any>
#include <variant>

namespace scada::opcua_bridge {

opcua::ServiceContext ToOpcua(const scada::ServiceContext& c) {
  return opcua::ServiceContext{}
      .with_user_id(ToOpcua(c.user_id()))
      .with_user_rights(c.user_rights())
      .with_request_id(c.request_id())
      .with_trace_id(c.trace_id())
      .with_peer(c.peer());
}
scada::ServiceContext ToScada(const opcua::ServiceContext& c) {
  return scada::ServiceContext{}
      .with_user_id(ToScada(c.user_id()))
      .with_user_rights(c.user_rights())
      .with_request_id(c.request_id())
      .with_trace_id(c.trace_id())
      .with_peer(c.peer());
}

opcua::ReadValueId ToOpcua(const scada::ReadValueId& v) {
  return {.node_id = ToOpcua(v.node_id),
          .attribute_id = ToOpcua(v.attribute_id)};
}
scada::ReadValueId ToScada(const opcua::ReadValueId& v) {
  return {.node_id = ToScada(v.node_id),
          .attribute_id = ToScada(v.attribute_id)};
}

opcua::WriteValue ToOpcua(const scada::WriteValue& v) {
  return {.node_id = ToOpcua(v.node_id),
          .attribute_id = ToOpcua(v.attribute_id),
          .value = ToOpcua(v.value),
          .flags = ToOpcua(v.flags)};
}
scada::WriteValue ToScada(const opcua::WriteValue& v) {
  return {.node_id = ToScada(v.node_id),
          .attribute_id = ToScada(v.attribute_id),
          .value = ToScada(v.value),
          .flags = ToScada(v.flags)};
}

opcua::BrowseDescription ToOpcua(const scada::BrowseDescription& v) {
  return {.node_id = ToOpcua(v.node_id),
          .direction = ToOpcua(v.direction),
          .reference_type_id = ToOpcua(v.reference_type_id),
          .include_subtypes = v.include_subtypes,
          .node_class_mask = v.node_class_mask,
          .result_mask = v.result_mask};
}
scada::BrowseDescription ToScada(const opcua::BrowseDescription& v) {
  return {.node_id = ToScada(v.node_id),
          .direction = ToScada(v.direction),
          .reference_type_id = ToScada(v.reference_type_id),
          .include_subtypes = v.include_subtypes,
          .node_class_mask = v.node_class_mask,
          .result_mask = v.result_mask};
}

opcua::ReferenceDescription ToOpcua(const scada::ReferenceDescription& v) {
  return {.reference_type_id = ToOpcua(v.reference_type_id),
          .forward = v.forward,
          .node_id = ToOpcua(v.node_id),
          .node_class = ToOpcua(v.node_class),
          .browse_name = ToOpcua(v.browse_name),
          .display_name = v.display_name,  // LocalizedText is std::u16string
          .type_definition = ToOpcua(v.type_definition)};
}
scada::ReferenceDescription ToScada(const opcua::ReferenceDescription& v) {
  return {.reference_type_id = ToScada(v.reference_type_id),
          .forward = v.forward,
          .node_id = ToScada(v.node_id),
          .node_class = ToScada(v.node_class),
          .browse_name = ToScada(v.browse_name),
          .display_name = v.display_name,
          .type_definition = ToScada(v.type_definition)};
}

opcua::BrowseResult ToOpcua(const scada::BrowseResult& v) {
  return {.status_code = ToOpcua(v.status_code),
          .continuation_point = v.continuation_point,
          .references = ToOpcuaVector(v.references)};
}
scada::BrowseResult ToScada(const opcua::BrowseResult& v) {
  return {.status_code = ToScada(v.status_code),
          .continuation_point = v.continuation_point,
          .references = ToScadaVector(v.references)};
}

opcua::RelativePathElement ToOpcua(const scada::RelativePathElement& v) {
  return {.reference_type_id = ToOpcua(v.reference_type_id),
          .inverse = v.inverse,
          .include_subtypes = v.include_subtypes,
          .target_name = ToOpcua(v.target_name)};
}
scada::RelativePathElement ToScada(const opcua::RelativePathElement& v) {
  return {.reference_type_id = ToScada(v.reference_type_id),
          .inverse = v.inverse,
          .include_subtypes = v.include_subtypes,
          .target_name = ToScada(v.target_name)};
}

opcua::BrowsePath ToOpcua(const scada::BrowsePath& v) {
  return {.node_id = ToOpcua(v.node_id),
          .relative_path = ToOpcuaVector(v.relative_path)};
}
scada::BrowsePath ToScada(const opcua::BrowsePath& v) {
  return {.node_id = ToScada(v.node_id),
          .relative_path = ToScadaVector(v.relative_path)};
}

opcua::BrowsePathTarget ToOpcua(const scada::BrowsePathTarget& v) {
  return {.target_id = ToOpcua(v.target_id),
          .remaining_path_index = v.remaining_path_index};
}
scada::BrowsePathTarget ToScada(const opcua::BrowsePathTarget& v) {
  return {.target_id = ToScada(v.target_id),
          .remaining_path_index = v.remaining_path_index};
}

opcua::BrowsePathResult ToOpcua(const scada::BrowsePathResult& v) {
  return {.status_code = ToOpcua(v.status_code),
          .targets = ToOpcuaVector(v.targets)};
}
scada::BrowsePathResult ToScada(const opcua::BrowsePathResult& v) {
  return {.status_code = ToScada(v.status_code),
          .targets = ToScadaVector(v.targets)};
}

opcua::NodeAttributes ToOpcua(const scada::NodeAttributes& v) {
  opcua::NodeAttributes out;
  out.browse_name = ToOpcua(v.browse_name);
  out.display_name = v.display_name;  // LocalizedText is std::u16string
  out.data_type = ToOpcua(v.data_type);
  out.value = ToOpcua(v.value);
  return out;
}
scada::NodeAttributes ToScada(const opcua::NodeAttributes& v) {
  scada::NodeAttributes out;
  out.browse_name = ToScada(v.browse_name);
  out.display_name = v.display_name;
  out.data_type = ToScada(v.data_type);
  out.value = ToScada(v.value);
  return out;
}

opcua::AddNodesItem ToOpcua(const scada::AddNodesItem& v) {
  return {.requested_id = ToOpcua(v.requested_id),
          .parent_id = ToOpcua(v.parent_id),
          .node_class = ToOpcua(v.node_class),
          .type_definition_id = ToOpcua(v.type_definition_id),
          .attributes = ToOpcua(v.attributes)};
}
scada::AddNodesItem ToScada(const opcua::AddNodesItem& v) {
  return {.requested_id = ToScada(v.requested_id),
          .parent_id = ToScada(v.parent_id),
          .node_class = ToScada(v.node_class),
          .type_definition_id = ToScada(v.type_definition_id),
          .attributes = ToScada(v.attributes)};
}

opcua::AddNodesResult ToOpcua(const scada::AddNodesResult& v) {
  return {.status_code = ToOpcua(v.status_code),
          .added_node_id = ToOpcua(v.added_node_id)};
}
scada::AddNodesResult ToScada(const opcua::AddNodesResult& v) {
  return {.status_code = ToScada(v.status_code),
          .added_node_id = ToScada(v.added_node_id)};
}

opcua::DeleteNodesItem ToOpcua(const scada::DeleteNodesItem& v) {
  return {.node_id = ToOpcua(v.node_id),
          .delete_target_references = v.delete_target_references};
}
scada::DeleteNodesItem ToScada(const opcua::DeleteNodesItem& v) {
  return {.node_id = ToScada(v.node_id),
          .delete_target_references = v.delete_target_references};
}

opcua::AddReferencesItem ToOpcua(const scada::AddReferencesItem& v) {
  return {.source_node_id = ToOpcua(v.source_node_id),
          .reference_type_id = ToOpcua(v.reference_type_id),
          .forward = v.forward,
          .target_server_uri = v.target_server_uri,
          .target_node_id = ToOpcua(v.target_node_id),
          .target_node_class = ToOpcua(v.target_node_class)};
}
scada::AddReferencesItem ToScada(const opcua::AddReferencesItem& v) {
  return {.source_node_id = ToScada(v.source_node_id),
          .reference_type_id = ToScada(v.reference_type_id),
          .forward = v.forward,
          .target_server_uri = v.target_server_uri,
          .target_node_id = ToScada(v.target_node_id),
          .target_node_class = ToScada(v.target_node_class)};
}

opcua::DeleteReferencesItem ToOpcua(const scada::DeleteReferencesItem& v) {
  return {.source_node_id = ToOpcua(v.source_node_id),
          .reference_type_id = ToOpcua(v.reference_type_id),
          .forward = v.forward,
          .target_node_id = ToOpcua(v.target_node_id),
          .delete_bidirectional = v.delete_bidirectional};
}
scada::DeleteReferencesItem ToScada(const opcua::DeleteReferencesItem& v) {
  return {.source_node_id = ToScada(v.source_node_id),
          .reference_type_id = ToScada(v.reference_type_id),
          .forward = v.forward,
          .target_node_id = ToScada(v.target_node_id),
          .delete_bidirectional = v.delete_bidirectional};
}

opcua::DataChangeFilter ToOpcua(const scada::DataChangeFilter& v) {
  // Core's DataChangeFilter only carries the deadband magnitude. The wire type
  // additionally encodes the trigger and deadband type; a non-zero deadband
  // implies an absolute deadband, which is what the server-side deadband logic
  // (DataChangeFilter §7.22.2) checks for.
  return {.trigger = opcua::DataChangeTrigger::StatusValue,
          .deadband_type = v.deadband_value > 0 ? opcua::DeadbandType::Absolute
                                                : opcua::DeadbandType::None,
          .deadband_value = v.deadband_value};
}
scada::DataChangeFilter ToScada(const opcua::DataChangeFilter& v) {
  return {.deadband_value = v.deadband_value};
}

opcua::AggregateFilter ToOpcua(const scada::AggregateFilter& v) {
  return {.start_time = ToOpcua(v.start_time),
          .interval = ToOpcua(v.interval),
          .aggregate_type = ToOpcua(v.aggregate_type)};
}
scada::AggregateFilter ToScada(const opcua::AggregateFilter& v) {
  return {.start_time = ToScada(v.start_time),
          .interval = ToScada(v.interval),
          .aggregate_type = ToScada(v.aggregate_type)};
}

opcua::EventFilter ToOpcua(const scada::EventFilter& v) {
  opcua::EventFilter out;
  out.types = v.types;
  out.of_type = ToOpcuaVector(v.of_type);
  out.child_of = ToOpcuaVector(v.child_of);
  return out;
}
scada::EventFilter ToScada(const opcua::EventFilter& v) {
  scada::EventFilter out;
  out.types = v.types;
  out.of_type = ToScadaVector(v.of_type);
  out.child_of = ToScadaVector(v.child_of);
  return out;
}

namespace {

// Maps a core MonitoringFilter to/from the wire filter. The wire
// MonitoringFilter is variant<DataChangeFilter, boost::json::value>. A core
// DataChangeFilter maps to the wire DataChangeFilter. Core event/aggregate
// filters use SCADA's own model (event-type/node selection; aggregate
// type/interval) rather than the OPC UA SelectClauses shape, so the bridge
// serializes them into a self-describing json object tagged with "_scada". Both
// ends of the SCADA path are the bridge, and opcuapp transports the json
// opaquely (ParseEventFilterFieldPaths finds no SelectClauses and yields no
// field projection, which matches SCADA event semantics). An absent (monostate)
// core filter maps to a missing wire filter.

boost::json::value EventFilterToJson(const scada::EventFilter& v) {
  boost::json::array of_type;
  for (const auto& id : v.of_type)
    of_type.emplace_back(id.ToString());
  boost::json::array child_of;
  for (const auto& id : v.child_of)
    child_of.emplace_back(id.ToString());
  return boost::json::object{{"_scada", "event"},
                             {"types", v.types},
                             {"of_type", std::move(of_type)},
                             {"child_of", std::move(child_of)}};
}

scada::EventFilter EventFilterFromJson(const boost::json::object& obj) {
  scada::EventFilter out;
  out.types = static_cast<unsigned>(obj.at("types").to_number<std::uint64_t>());
  for (const auto& id : obj.at("of_type").as_array())
    out.of_type.push_back(scada::NodeId::FromString(id.as_string().c_str()));
  for (const auto& id : obj.at("child_of").as_array())
    out.child_of.push_back(scada::NodeId::FromString(id.as_string().c_str()));
  return out;
}

boost::json::value AggregateFilterToJson(const scada::AggregateFilter& v) {
  return boost::json::object{
      {"_scada", "aggregate"},
      {"start_time_us",
       v.start_time.ToDeltaSinceWindowsEpoch().InMicroseconds()},
      {"interval_us", v.interval.InMicroseconds()},
      {"aggregate_type", v.aggregate_type.ToString()}};
}

scada::AggregateFilter AggregateFilterFromJson(const boost::json::object& obj) {
  scada::AggregateFilter out;
  out.start_time =
      base::Time::FromDeltaSinceWindowsEpoch(base::TimeDelta::FromMicroseconds(
          obj.at("start_time_us").to_number<std::int64_t>()));
  out.interval = base::TimeDelta::FromMicroseconds(
      obj.at("interval_us").to_number<std::int64_t>());
  out.aggregate_type =
      scada::NodeId::FromString(obj.at("aggregate_type").as_string().c_str());
  return out;
}

std::optional<opcua::MonitoringFilter> ToOpcuaFilter(
    const scada::MonitoringFilter& f) {
  return std::visit(
      [](const auto& x) -> std::optional<opcua::MonitoringFilter> {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return std::nullopt;
        else if constexpr (std::is_same_v<T, scada::DataChangeFilter>)
          return opcua::MonitoringFilter{ToOpcua(x)};
        else if constexpr (std::is_same_v<T, scada::EventFilter>)
          return opcua::MonitoringFilter{EventFilterToJson(x)};
        else  // scada::AggregateFilter
          return opcua::MonitoringFilter{AggregateFilterToJson(x)};
      },
      f);
}
scada::MonitoringFilter ToScadaFilter(
    const std::optional<opcua::MonitoringFilter>& f) {
  if (!f.has_value())
    return std::monostate{};
  if (const auto* data_change = std::get_if<opcua::DataChangeFilter>(&*f))
    return ToScada(*data_change);
  const auto& json = std::get<boost::json::value>(*f);
  if (json.is_object()) {
    const auto& obj = json.as_object();
    if (const auto* tag = obj.if_contains("_scada");
        tag != nullptr && tag->is_string()) {
      if (tag->as_string() == "event")
        return EventFilterFromJson(obj);
      if (tag->as_string() == "aggregate")
        return AggregateFilterFromJson(obj);
    }
  }
  // Opaque/standard json filter (e.g. OPC UA SelectClauses) we do not map back
  // to a typed core filter.
  return std::monostate{};
}
}  // namespace

opcua::MonitoringParameters ToOpcua(const scada::MonitoringParameters& v) {
  opcua::MonitoringParameters out;
  out.sampling_interval_ms = v.sampling_interval.has_value()
                                 ? v.sampling_interval->InMillisecondsF()
                                 : 0.0;
  out.filter = ToOpcuaFilter(v.filter);
  out.queue_size = static_cast<opcua::UInt32>(v.queue_size.value_or(1));
  return out;
}
scada::MonitoringParameters ToScada(const opcua::MonitoringParameters& v) {
  scada::MonitoringParameters out;
  out.sampling_interval =
      base::TimeDelta::FromMillisecondsD(v.sampling_interval_ms);
  out.filter = ToScadaFilter(v.filter);
  out.queue_size = static_cast<size_t>(v.queue_size);
  return out;
}

opcua::Event ToOpcua(const scada::Event& v) {
  opcua::Event out;
  out.event_type_id = ToOpcua(v.event_type_id);
  out.event_id = v.event_id;
  out.time = ToOpcua(v.time);
  out.receive_time = ToOpcua(v.receive_time);
  out.change_mask = v.change_mask;
  out.severity = v.severity;
  out.node_id = ToOpcua(v.node_id);
  out.user_id = ToOpcua(v.user_id);
  out.value = ToOpcua(v.value);
  out.qualifier = ToOpcua(v.qualifier);
  out.message = v.message;  // LocalizedText is std::u16string
  out.acked = v.acked;
  out.acknowledged_time = ToOpcua(v.acknowledged_time);
  out.acknowledged_user_id = ToOpcua(v.acknowledged_user_id);
  return out;
}
scada::Event ToScada(const opcua::Event& v) {
  scada::Event out;
  out.event_type_id = ToScada(v.event_type_id);
  out.event_id = v.event_id;
  out.time = ToScada(v.time);
  out.receive_time = ToScada(v.receive_time);
  out.change_mask = v.change_mask;
  out.severity = v.severity;
  out.node_id = ToScada(v.node_id);
  out.user_id = ToScada(v.user_id);
  out.value = ToScada(v.value);
  out.qualifier = ToScada(v.qualifier);
  out.message = v.message;
  out.acked = v.acked;
  out.acknowledged_time = ToScada(v.acknowledged_time);
  out.acknowledged_user_id = ToScada(v.acknowledged_user_id);
  return out;
}

opcua::HistoryReadRawDetails ToOpcua(const scada::HistoryReadRawDetails& v) {
  return {.node_id = ToOpcua(v.node_id),
          .from = ToOpcua(v.from),
          .to = ToOpcua(v.to),
          .max_count = v.max_count,
          .aggregation = ToOpcua(v.aggregation),
          .release_continuation_point = v.release_continuation_point,
          .continuation_point = v.continuation_point};
}
scada::HistoryReadRawDetails ToScada(const opcua::HistoryReadRawDetails& v) {
  return {.node_id = ToScada(v.node_id),
          .from = ToScada(v.from),
          .to = ToScada(v.to),
          .max_count = v.max_count,
          .aggregation = ToScada(v.aggregation),
          .release_continuation_point = v.release_continuation_point,
          .continuation_point = v.continuation_point};
}

opcua::HistoryReadEventsDetails ToOpcua(
    const scada::HistoryReadEventsDetails& v) {
  return {.node_id = ToOpcua(v.node_id),
          .from = ToOpcua(v.from),
          .to = ToOpcua(v.to),
          .filter = ToOpcua(v.filter)};
}
scada::HistoryReadEventsDetails ToScada(
    const opcua::HistoryReadEventsDetails& v) {
  return {.node_id = ToScada(v.node_id),
          .from = ToScada(v.from),
          .to = ToScada(v.to),
          .filter = ToScada(v.filter)};
}

opcua::HistoryReadRawResult ToOpcua(const scada::HistoryReadRawResult& v) {
  return {.status = ToOpcua(v.status),
          .values = ToOpcuaVector(v.values),
          .continuation_point = v.continuation_point};
}
scada::HistoryReadRawResult ToScada(const opcua::HistoryReadRawResult& v) {
  return {.status = ToScada(v.status),
          .values = ToScadaVector(v.values),
          .continuation_point = v.continuation_point};
}

opcua::HistoryReadEventsResult ToOpcua(
    const scada::HistoryReadEventsResult& v) {
  return {.status = ToOpcua(v.status), .events = ToOpcuaVector(v.events)};
}
scada::HistoryReadEventsResult ToScada(
    const opcua::HistoryReadEventsResult& v) {
  return {.status = ToScada(v.status), .events = ToScadaVector(v.events)};
}

opcua::UpdateDataDetails ToOpcua(const scada::UpdateDataDetails& v) {
  // PerformUpdateType is the OPC UA Part 11 §6.8.3 enumeration on both sides
  // with identical wire values, so the cast is value-preserving.
  return {.node_id = ToOpcua(v.node_id),
          .perform_insert_replace =
              static_cast<opcua::PerformUpdateType>(v.perform_insert_replace),
          .values = ToOpcuaVector(v.values)};
}
scada::UpdateDataDetails ToScada(const opcua::UpdateDataDetails& v) {
  return {.node_id = ToScada(v.node_id),
          .perform_insert_replace =
              static_cast<scada::PerformUpdateType>(v.perform_insert_replace),
          .values = ToScadaVector(v.values)};
}

opcua::UpdateEventDetails ToOpcua(const scada::UpdateEventDetails& v) {
  return {.node_id = ToOpcua(v.node_id),
          .perform_insert_replace =
              static_cast<opcua::PerformUpdateType>(v.perform_insert_replace),
          .events = ToOpcuaVector(v.events)};
}
scada::UpdateEventDetails ToScada(const opcua::UpdateEventDetails& v) {
  return {.node_id = ToScada(v.node_id),
          .perform_insert_replace =
              static_cast<scada::PerformUpdateType>(v.perform_insert_replace),
          .events = ToScadaVector(v.events)};
}

opcua::MonitoredItemSubscriptionOptions ToOpcua(
    const scada::MonitoredItemSubscriptionOptions& v) {
  return {.max_pending_notifications = v.max_pending_notifications,
          .max_batch_size = v.max_batch_size};
}
scada::MonitoredItemSubscriptionOptions ToScada(
    const opcua::MonitoredItemSubscriptionOptions& v) {
  return {.max_pending_notifications = v.max_pending_notifications,
          .max_batch_size = v.max_batch_size};
}

opcua::MonitoredItemCreateRequest ToOpcua(
    const scada::MonitoredItemCreateRequest& v) {
  // The wire request carries the per-item client_handle inside its
  // requested_parameters.
  opcua::MonitoringParameters parameters = ToOpcua(v.parameters);
  parameters.client_handle = v.client_handle;
  return {.item_to_monitor = ToOpcua(v.item_to_monitor),
          .monitoring_mode = opcua::MonitoringMode::Reporting,
          .requested_parameters = std::move(parameters)};
}
scada::MonitoredItemCreateRequest ToScada(
    const opcua::MonitoredItemCreateRequest& v) {
  return {.item_to_monitor = ToScada(v.item_to_monitor),
          .parameters = ToScada(v.requested_parameters),
          .client_handle = v.requested_parameters.client_handle};
}

opcua::MonitoredItemCreateResult ToOpcua(
    const scada::MonitoredItemCreateResult& v) {
  // The wire result has no client_handle field; it is correlated by request
  // order on the caller side.
  return {.status = ToOpcua(v.status), .monitored_item_id = v.item_id};
}
scada::MonitoredItemCreateResult ToScada(
    const opcua::MonitoredItemCreateResult& v) {
  return {.item_id = v.monitored_item_id, .status = ToScada(v.status)};
}

scada::MonitoredItemNotification ToScada(const opcua::ItemNotification& n) {
  // The wire notification is one of two standard types. A
  // MonitoredItemNotification (client_handle + DataValue) maps to a core
  // DataChangeNotification. An EventFieldList (client_handle + projected event
  // fields) maps to a core EventNotification; the std::any payload is
  // reassembled from the event fields via the core AssembleEvent. Consumers
  // correlate by client_handle.
  return std::visit(
      [](const auto& x) -> scada::MonitoredItemNotification {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, opcua::MonitoredItemNotification>) {
          return scada::DataChangeNotification{.item_id = 0,
                                               .client_handle = x.client_handle,
                                               .value = ToScada(x.value)};
        } else {  // opcua::EventFieldList
          // Reassemble the core event std::any from the wire EventFieldList.
          // OPC UA Part 4 §7.22.3 EventFilter delivers each Event as the field
          // values selected by the SelectClauses,
          // https://reference.opcfoundation.org/Core/Part4/v105/docs/7.22 .
          // Two layouts arrive on the SCADA path: the model-notification
          // payloads (GeneralModelChange / SemanticChange) cross as their
          // per-type DisassembleEvent layout, which AssembleEvent rebuilds by
          // the type id at field 0; a scada::Event payload crosses as the
          // DEFAULT full-fidelity projection (the _scada event filter carries
          // no SelectClauses, so the serving side projects
          // DefaultEventFieldPaths) and is rebuilt the same way the
          // HistoryReadEvents decode rebuilds it. An unrecognized layout
          // yields an empty payload.
          std::vector<scada::Variant> fields = ToScadaVector(x.event_fields);
          std::any event =
              fields.empty() ? std::any{} : scada::AssembleEvent(fields);
          if (!event.has_value() &&
              x.event_fields.size() == opcua::DefaultEventFieldPaths().size()) {
            event = std::any{ToScada(opcua::ReconstructEventFromFields(
                opcua::DefaultEventFieldPaths(), x.event_fields))};
          }
          return scada::EventNotification{.item_id = 0,
                                          .client_handle = x.client_handle,
                                          .status = scada::StatusCode::Good,
                                          .event = std::move(event)};
        }
      },
      n);
}

opcua::SessionSecuritySettings ToOpcua(
    const scada::SessionSecuritySettings& v) {
  return {.mode = static_cast<opcua::SessionSecuritySettings::Mode>(v.mode),
          .required_policy_uri = v.required_policy_uri,
          .client_certificate_path = v.client_certificate_path,
          .client_private_key_path = v.client_private_key_path};
}
scada::SessionSecuritySettings ToScada(
    const opcua::SessionSecuritySettings& v) {
  return {.mode = static_cast<scada::SessionSecuritySettings::Mode>(v.mode),
          .required_policy_uri = v.required_policy_uri,
          .client_certificate_path = v.client_certificate_path,
          .client_private_key_path = v.client_private_key_path};
}

opcua::SessionConnectParams ToOpcua(const scada::SessionConnectParams& v) {
  return {.host = v.host,
          .connection_string = v.connection_string,
          .user_name = v.user_name,  // LocalizedText is std::u16string
          .password = v.password,
          .allow_remote_logoff = v.allow_remote_logoff,
          .security = ToOpcua(v.security)};
}
scada::SessionConnectParams ToScada(const opcua::SessionConnectParams& v) {
  return {.host = v.host,
          .connection_string = v.connection_string,
          .user_name = v.user_name,
          .password = v.password,
          .allow_remote_logoff = v.allow_remote_logoff,
          .security = ToScada(v.security)};
}

}  // namespace scada::opcua_bridge
