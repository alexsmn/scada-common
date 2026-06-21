#include "opcua_bridge/service_conversion.h"

#include "opcua_bridge/vector_conversion.h"

#include <variant>

namespace opcua_bridge {

opcua::scada::ServiceContext ToOpcua(const scada::ServiceContext& c) {
  return opcua::scada::ServiceContext{}
      .with_user_id(ToOpcua(c.user_id()))
      .with_request_id(c.request_id())
      .with_trace_id(c.trace_id());
}
scada::ServiceContext ToScada(const opcua::scada::ServiceContext& c) {
  return scada::ServiceContext{}
      .with_user_id(ToScada(c.user_id()))
      .with_request_id(c.request_id())
      .with_trace_id(c.trace_id());
}

opcua::scada::ReadValueId ToOpcua(const scada::ReadValueId& v) {
  return {.node_id = ToOpcua(v.node_id), .attribute_id = ToOpcua(v.attribute_id)};
}
scada::ReadValueId ToScada(const opcua::scada::ReadValueId& v) {
  return {.node_id = ToScada(v.node_id), .attribute_id = ToScada(v.attribute_id)};
}

opcua::scada::WriteValue ToOpcua(const scada::WriteValue& v) {
  return {.node_id = ToOpcua(v.node_id),
          .attribute_id = ToOpcua(v.attribute_id),
          .value = ToOpcua(v.value),
          .flags = ToOpcua(v.flags)};
}
scada::WriteValue ToScada(const opcua::scada::WriteValue& v) {
  return {.node_id = ToScada(v.node_id),
          .attribute_id = ToScada(v.attribute_id),
          .value = ToScada(v.value),
          .flags = ToScada(v.flags)};
}

opcua::scada::BrowseDescription ToOpcua(const scada::BrowseDescription& v) {
  return {.node_id = ToOpcua(v.node_id),
          .direction = ToOpcua(v.direction),
          .reference_type_id = ToOpcua(v.reference_type_id),
          .include_subtypes = v.include_subtypes,
          .node_class_mask = v.node_class_mask,
          .result_mask = v.result_mask};
}
scada::BrowseDescription ToScada(const opcua::scada::BrowseDescription& v) {
  return {.node_id = ToScada(v.node_id),
          .direction = ToScada(v.direction),
          .reference_type_id = ToScada(v.reference_type_id),
          .include_subtypes = v.include_subtypes,
          .node_class_mask = v.node_class_mask,
          .result_mask = v.result_mask};
}

opcua::scada::ReferenceDescription ToOpcua(const scada::ReferenceDescription& v) {
  return {.reference_type_id = ToOpcua(v.reference_type_id),
          .forward = v.forward,
          .node_id = ToOpcua(v.node_id),
          .node_class = ToOpcua(v.node_class),
          .browse_name = ToOpcua(v.browse_name),
          .display_name = v.display_name,  // LocalizedText is std::u16string
          .type_definition = ToOpcua(v.type_definition)};
}
scada::ReferenceDescription ToScada(const opcua::scada::ReferenceDescription& v) {
  return {.reference_type_id = ToScada(v.reference_type_id),
          .forward = v.forward,
          .node_id = ToScada(v.node_id),
          .node_class = ToScada(v.node_class),
          .browse_name = ToScada(v.browse_name),
          .display_name = v.display_name,
          .type_definition = ToScada(v.type_definition)};
}

opcua::scada::BrowseResult ToOpcua(const scada::BrowseResult& v) {
  return {.status_code = ToOpcua(v.status_code),
          .continuation_point = v.continuation_point,
          .references = ToOpcuaVector(v.references)};
}
scada::BrowseResult ToScada(const opcua::scada::BrowseResult& v) {
  return {.status_code = ToScada(v.status_code),
          .continuation_point = v.continuation_point,
          .references = ToScadaVector(v.references)};
}

opcua::scada::RelativePathElement ToOpcua(const scada::RelativePathElement& v) {
  return {.reference_type_id = ToOpcua(v.reference_type_id),
          .inverse = v.inverse,
          .include_subtypes = v.include_subtypes,
          .target_name = ToOpcua(v.target_name)};
}
scada::RelativePathElement ToScada(const opcua::scada::RelativePathElement& v) {
  return {.reference_type_id = ToScada(v.reference_type_id),
          .inverse = v.inverse,
          .include_subtypes = v.include_subtypes,
          .target_name = ToScada(v.target_name)};
}

opcua::scada::BrowsePath ToOpcua(const scada::BrowsePath& v) {
  return {.node_id = ToOpcua(v.node_id),
          .relative_path = ToOpcuaVector(v.relative_path)};
}
scada::BrowsePath ToScada(const opcua::scada::BrowsePath& v) {
  return {.node_id = ToScada(v.node_id),
          .relative_path = ToScadaVector(v.relative_path)};
}

opcua::scada::BrowsePathTarget ToOpcua(const scada::BrowsePathTarget& v) {
  return {.target_id = ToOpcua(v.target_id),
          .remaining_path_index = v.remaining_path_index};
}
scada::BrowsePathTarget ToScada(const opcua::scada::BrowsePathTarget& v) {
  return {.target_id = ToScada(v.target_id),
          .remaining_path_index = v.remaining_path_index};
}

opcua::scada::BrowsePathResult ToOpcua(const scada::BrowsePathResult& v) {
  return {.status_code = ToOpcua(v.status_code),
          .targets = ToOpcuaVector(v.targets)};
}
scada::BrowsePathResult ToScada(const opcua::scada::BrowsePathResult& v) {
  return {.status_code = ToScada(v.status_code),
          .targets = ToScadaVector(v.targets)};
}

opcua::scada::NodeAttributes ToOpcua(const scada::NodeAttributes& v) {
  opcua::scada::NodeAttributes out;
  out.browse_name = ToOpcua(v.browse_name);
  out.display_name = v.display_name;  // LocalizedText is std::u16string
  out.data_type = ToOpcua(v.data_type);
  out.value = ToOpcua(v.value);
  return out;
}
scada::NodeAttributes ToScada(const opcua::scada::NodeAttributes& v) {
  scada::NodeAttributes out;
  out.browse_name = ToScada(v.browse_name);
  out.display_name = v.display_name;
  out.data_type = ToScada(v.data_type);
  out.value = ToScada(v.value);
  return out;
}

opcua::scada::AddNodesItem ToOpcua(const scada::AddNodesItem& v) {
  return {.requested_id = ToOpcua(v.requested_id),
          .parent_id = ToOpcua(v.parent_id),
          .node_class = ToOpcua(v.node_class),
          .type_definition_id = ToOpcua(v.type_definition_id),
          .attributes = ToOpcua(v.attributes)};
}
scada::AddNodesItem ToScada(const opcua::scada::AddNodesItem& v) {
  return {.requested_id = ToScada(v.requested_id),
          .parent_id = ToScada(v.parent_id),
          .node_class = ToScada(v.node_class),
          .type_definition_id = ToScada(v.type_definition_id),
          .attributes = ToScada(v.attributes)};
}

opcua::scada::AddNodesResult ToOpcua(const scada::AddNodesResult& v) {
  return {.status_code = ToOpcua(v.status_code),
          .added_node_id = ToOpcua(v.added_node_id)};
}
scada::AddNodesResult ToScada(const opcua::scada::AddNodesResult& v) {
  return {.status_code = ToScada(v.status_code),
          .added_node_id = ToScada(v.added_node_id)};
}

opcua::scada::DeleteNodesItem ToOpcua(const scada::DeleteNodesItem& v) {
  return {.node_id = ToOpcua(v.node_id),
          .delete_target_references = v.delete_target_references};
}
scada::DeleteNodesItem ToScada(const opcua::scada::DeleteNodesItem& v) {
  return {.node_id = ToScada(v.node_id),
          .delete_target_references = v.delete_target_references};
}

opcua::scada::AddReferencesItem ToOpcua(const scada::AddReferencesItem& v) {
  return {.source_node_id = ToOpcua(v.source_node_id),
          .reference_type_id = ToOpcua(v.reference_type_id),
          .forward = v.forward,
          .target_server_uri = v.target_server_uri,
          .target_node_id = ToOpcua(v.target_node_id),
          .target_node_class = ToOpcua(v.target_node_class)};
}
scada::AddReferencesItem ToScada(const opcua::scada::AddReferencesItem& v) {
  return {.source_node_id = ToScada(v.source_node_id),
          .reference_type_id = ToScada(v.reference_type_id),
          .forward = v.forward,
          .target_server_uri = v.target_server_uri,
          .target_node_id = ToScada(v.target_node_id),
          .target_node_class = ToScada(v.target_node_class)};
}

opcua::scada::DeleteReferencesItem ToOpcua(const scada::DeleteReferencesItem& v) {
  return {.source_node_id = ToOpcua(v.source_node_id),
          .reference_type_id = ToOpcua(v.reference_type_id),
          .forward = v.forward,
          .target_node_id = ToOpcua(v.target_node_id),
          .delete_bidirectional = v.delete_bidirectional};
}
scada::DeleteReferencesItem ToScada(const opcua::scada::DeleteReferencesItem& v) {
  return {.source_node_id = ToScada(v.source_node_id),
          .reference_type_id = ToScada(v.reference_type_id),
          .forward = v.forward,
          .target_node_id = ToScada(v.target_node_id),
          .delete_bidirectional = v.delete_bidirectional};
}

opcua::scada::DataChangeFilter ToOpcua(const scada::DataChangeFilter& v) {
  return {.deadband_value = v.deadband_value};
}
scada::DataChangeFilter ToScada(const opcua::scada::DataChangeFilter& v) {
  return {.deadband_value = v.deadband_value};
}

opcua::scada::AggregateFilter ToOpcua(const scada::AggregateFilter& v) {
  return {.start_time = ToOpcua(v.start_time),
          .interval = ToOpcua(v.interval),
          .aggregate_type = ToOpcua(v.aggregate_type)};
}
scada::AggregateFilter ToScada(const opcua::scada::AggregateFilter& v) {
  return {.start_time = ToScada(v.start_time),
          .interval = ToScada(v.interval),
          .aggregate_type = ToScada(v.aggregate_type)};
}

opcua::scada::EventFilter ToOpcua(const scada::EventFilter& v) {
  opcua::scada::EventFilter out;
  out.types = v.types;
  out.of_type = ToOpcuaVector(v.of_type);
  out.child_of = ToOpcuaVector(v.child_of);
  return out;
}
scada::EventFilter ToScada(const opcua::scada::EventFilter& v) {
  scada::EventFilter out;
  out.types = v.types;
  out.of_type = ToScadaVector(v.of_type);
  out.child_of = ToScadaVector(v.child_of);
  return out;
}

namespace {
opcua::scada::MonitoringFilter ToOpcuaFilter(const scada::MonitoringFilter& f) {
  return std::visit(
      [](const auto& x) -> opcua::scada::MonitoringFilter {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return std::monostate{};
        else
          return ToOpcua(x);
      },
      f);
}
scada::MonitoringFilter ToScadaFilter(const opcua::scada::MonitoringFilter& f) {
  return std::visit(
      [](const auto& x) -> scada::MonitoringFilter {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, std::monostate>)
          return std::monostate{};
        else
          return ToScada(x);
      },
      f);
}
}  // namespace

opcua::scada::MonitoringParameters ToOpcua(const scada::MonitoringParameters& v) {
  opcua::scada::MonitoringParameters out;
  out.sampling_interval = ToOpcua(v.sampling_interval);
  out.filter = ToOpcuaFilter(v.filter);
  out.queue_size = v.queue_size;
  return out;
}
scada::MonitoringParameters ToScada(const opcua::scada::MonitoringParameters& v) {
  scada::MonitoringParameters out;
  out.sampling_interval = ToScada(v.sampling_interval);
  out.filter = ToScadaFilter(v.filter);
  out.queue_size = v.queue_size;
  return out;
}

opcua::scada::Event ToOpcua(const scada::Event& v) {
  opcua::scada::Event out;
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
scada::Event ToScada(const opcua::scada::Event& v) {
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

opcua::scada::HistoryReadRawDetails ToOpcua(
    const scada::HistoryReadRawDetails& v) {
  return {.node_id = ToOpcua(v.node_id),
          .from = ToOpcua(v.from),
          .to = ToOpcua(v.to),
          .max_count = v.max_count,
          .aggregation = ToOpcua(v.aggregation),
          .release_continuation_point = v.release_continuation_point,
          .continuation_point = v.continuation_point};
}
scada::HistoryReadRawDetails ToScada(
    const opcua::scada::HistoryReadRawDetails& v) {
  return {.node_id = ToScada(v.node_id),
          .from = ToScada(v.from),
          .to = ToScada(v.to),
          .max_count = v.max_count,
          .aggregation = ToScada(v.aggregation),
          .release_continuation_point = v.release_continuation_point,
          .continuation_point = v.continuation_point};
}

opcua::scada::HistoryReadEventsDetails ToOpcua(
    const scada::HistoryReadEventsDetails& v) {
  return {.node_id = ToOpcua(v.node_id),
          .from = ToOpcua(v.from),
          .to = ToOpcua(v.to),
          .filter = ToOpcua(v.filter)};
}
scada::HistoryReadEventsDetails ToScada(
    const opcua::scada::HistoryReadEventsDetails& v) {
  return {.node_id = ToScada(v.node_id),
          .from = ToScada(v.from),
          .to = ToScada(v.to),
          .filter = ToScada(v.filter)};
}

opcua::scada::HistoryReadRawResult ToOpcua(
    const scada::HistoryReadRawResult& v) {
  return {.status = ToOpcua(v.status),
          .values = ToOpcuaVector(v.values),
          .continuation_point = v.continuation_point};
}
scada::HistoryReadRawResult ToScada(
    const opcua::scada::HistoryReadRawResult& v) {
  return {.status = ToScada(v.status),
          .values = ToScadaVector(v.values),
          .continuation_point = v.continuation_point};
}

opcua::scada::HistoryReadEventsResult ToOpcua(
    const scada::HistoryReadEventsResult& v) {
  return {.status = ToOpcua(v.status), .events = ToOpcuaVector(v.events)};
}
scada::HistoryReadEventsResult ToScada(
    const opcua::scada::HistoryReadEventsResult& v) {
  return {.status = ToScada(v.status), .events = ToScadaVector(v.events)};
}

opcua::scada::MonitoredItemSubscriptionOptions ToOpcua(
    const scada::MonitoredItemSubscriptionOptions& v) {
  return {.max_pending_notifications = v.max_pending_notifications,
          .max_batch_size = v.max_batch_size};
}
scada::MonitoredItemSubscriptionOptions ToScada(
    const opcua::scada::MonitoredItemSubscriptionOptions& v) {
  return {.max_pending_notifications = v.max_pending_notifications,
          .max_batch_size = v.max_batch_size};
}

opcua::scada::MonitoredItemCreateRequest ToOpcua(
    const scada::MonitoredItemCreateRequest& v) {
  return {.item_to_monitor = ToOpcua(v.item_to_monitor),
          .parameters = ToOpcua(v.parameters),
          .client_handle = v.client_handle};
}
scada::MonitoredItemCreateRequest ToScada(
    const opcua::scada::MonitoredItemCreateRequest& v) {
  return {.item_to_monitor = ToScada(v.item_to_monitor),
          .parameters = ToScada(v.parameters),
          .client_handle = v.client_handle};
}

opcua::scada::MonitoredItemCreateResult ToOpcua(
    const scada::MonitoredItemCreateResult& v) {
  return {.item_id = v.item_id,
          .client_handle = v.client_handle,
          .status = ToOpcua(v.status)};
}
scada::MonitoredItemCreateResult ToScada(
    const opcua::scada::MonitoredItemCreateResult& v) {
  return {.item_id = v.item_id,
          .client_handle = v.client_handle,
          .status = ToScada(v.status)};
}

opcua::scada::MonitoredItemNotification ToOpcua(
    const scada::MonitoredItemNotification& n) {
  return std::visit(
      [](const auto& x) -> opcua::scada::MonitoredItemNotification {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, scada::DataChangeNotification>)
          return opcua::scada::DataChangeNotification{
              .item_id = x.item_id,
              .client_handle = x.client_handle,
              .value = ToOpcua(x.value)};
        else if constexpr (std::is_same_v<T, scada::EventNotification>)
          // The event payload is a std::any that cannot cross the boundary by
          // value; the field is left empty (see ExtensionObject note).
          return opcua::scada::EventNotification{.item_id = x.item_id,
                                                 .client_handle = x.client_handle,
                                                 .status = ToOpcua(x.status),
                                                 .event = {}};
        else if constexpr (std::is_same_v<T, scada::ItemStatusNotification>)
          return opcua::scada::ItemStatusNotification{
              .item_id = x.item_id,
              .client_handle = x.client_handle,
              .status = ToOpcua(x.status)};
        else
          return opcua::scada::OverflowNotification{.status = ToOpcua(x.status)};
      },
      n);
}
scada::MonitoredItemNotification ToScada(
    const opcua::scada::MonitoredItemNotification& n) {
  return std::visit(
      [](const auto& x) -> scada::MonitoredItemNotification {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, opcua::scada::DataChangeNotification>)
          return scada::DataChangeNotification{.item_id = x.item_id,
                                               .client_handle = x.client_handle,
                                               .value = ToScada(x.value)};
        else if constexpr (std::is_same_v<T, opcua::scada::EventNotification>)
          return scada::EventNotification{.item_id = x.item_id,
                                          .client_handle = x.client_handle,
                                          .status = ToScada(x.status),
                                          .event = {}};
        else if constexpr (std::is_same_v<T, opcua::scada::ItemStatusNotification>)
          return scada::ItemStatusNotification{.item_id = x.item_id,
                                               .client_handle = x.client_handle,
                                               .status = ToScada(x.status)};
        else
          return scada::OverflowNotification{.status = ToScada(x.status)};
      },
      n);
}

opcua::scada::SessionSecuritySettings ToOpcua(
    const scada::SessionSecuritySettings& v) {
  return {.mode = static_cast<opcua::scada::SessionSecuritySettings::Mode>(v.mode),
          .required_policy_uri = v.required_policy_uri,
          .client_certificate_path = v.client_certificate_path,
          .client_private_key_path = v.client_private_key_path};
}
scada::SessionSecuritySettings ToScada(
    const opcua::scada::SessionSecuritySettings& v) {
  return {.mode = static_cast<scada::SessionSecuritySettings::Mode>(v.mode),
          .required_policy_uri = v.required_policy_uri,
          .client_certificate_path = v.client_certificate_path,
          .client_private_key_path = v.client_private_key_path};
}

opcua::scada::SessionConnectParams ToOpcua(
    const scada::SessionConnectParams& v) {
  return {.host = v.host,
          .connection_string = v.connection_string,
          .user_name = v.user_name,  // LocalizedText is std::u16string
          .password = v.password,
          .allow_remote_logoff = v.allow_remote_logoff,
          .security = ToOpcua(v.security)};
}
scada::SessionConnectParams ToScada(
    const opcua::scada::SessionConnectParams& v) {
  return {.host = v.host,
          .connection_string = v.connection_string,
          .user_name = v.user_name,
          .password = v.password,
          .allow_remote_logoff = v.allow_remote_logoff,
          .security = ToScada(v.security)};
}

}  // namespace opcua_bridge
