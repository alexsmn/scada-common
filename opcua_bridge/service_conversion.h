#pragma once

// Converters for the OPC UA service-interface structs (the parameter and result
// types of AttributeService / ViewService / MethodService /
// NodeManagementService / HistoryService / MonitoredItemService). Builds on the
// foundational converters in conversion.h; every struct here is a mechanical
// aggregate of those.

#include "opcua_bridge/conversion.h"

#include "scada/attribute_service.h"
#include "scada/authentication.h"
#include "scada/event.h"
#include "scada/history_types.h"
#include "scada/monitored_item.h"
#include "scada/monitoring_parameters.h"
#include "scada/node_attributes.h"
#include "scada/node_management_service.h"
#include "scada/privileges.h"
#include "scada/read_value_id.h"
#include "scada/service_context.h"
#include "scada/session_service.h"
#include "scada/view_service.h"
#include "scada/write_flags.h"

#include "opcua/events/event.h"
#include "opcua/message.h"
#include "opcua/monitored/monitored_item.h"
#include "opcua/services/attribute_types.h"
#include "opcua/services/history_types.h"
#include "opcua/services/node_management_types.h"
#include "opcua/services/service_context.h"
#include "opcua/services/view_types.h"
#include "opcua/session/authentication.h"
#include "opcua/session/session_types.h"
#include "opcua/types/node_attributes.h"
#include "opcua/types/privileges.h"
#include "opcua/types/read_value_id.h"
#include "opcua/types/write_flags.h"

namespace opcua_bridge {

// --- enums (identical underlying values) --------------------------------
inline opcua::AttributeId ToOpcua(scada::AttributeId v) {
  return static_cast<opcua::AttributeId>(v);
}
inline scada::AttributeId ToScada(opcua::AttributeId v) {
  return static_cast<scada::AttributeId>(v);
}
inline opcua::NodeClass ToOpcua(scada::NodeClass v) {
  return static_cast<opcua::NodeClass>(v);
}
inline scada::NodeClass ToScada(opcua::NodeClass v) {
  return static_cast<scada::NodeClass>(v);
}
inline opcua::BrowseDirection ToOpcua(scada::BrowseDirection v) {
  return static_cast<opcua::BrowseDirection>(v);
}
inline scada::BrowseDirection ToScada(opcua::BrowseDirection v) {
  return static_cast<scada::BrowseDirection>(v);
}
inline opcua::Privilege ToOpcua(scada::Privilege v) {
  return static_cast<opcua::Privilege>(v);
}
inline scada::Privilege ToScada(opcua::Privilege v) {
  return static_cast<scada::Privilege>(v);
}

// --- Duration (base::TimeDelta vs opcua::base::TimeDelta) ----------------
inline opcua::Duration ToOpcua(scada::Duration d) {
  return opcua::base::TimeDelta::FromInternalValue(d.ToInternalValue());
}
inline scada::Duration ToScada(opcua::Duration d) {
  return base::TimeDelta::FromInternalValue(d.ToInternalValue());
}

// --- WriteFlags ---------------------------------------------------------
inline opcua::WriteFlags ToOpcua(scada::WriteFlags v) {
  return opcua::WriteFlags{v.raw()};
}
inline scada::WriteFlags ToScada(opcua::WriteFlags v) {
  return scada::WriteFlags{v.raw()};
}

// --- optional<T> helper -------------------------------------------------
template <class T>
auto ToOpcua(const std::optional<T>& v)
    -> std::optional<std::decay_t<decltype(ToOpcua(*v))>> {
  if (!v)
    return std::nullopt;
  return ToOpcua(*v);
}
template <class T>
auto ToScada(const std::optional<T>& v)
    -> std::optional<std::decay_t<decltype(ToScada(*v))>> {
  if (!v)
    return std::nullopt;
  return ToScada(*v);
}

// --- service structs ----------------------------------------------------
opcua::ServiceContext ToOpcua(const scada::ServiceContext&);
scada::ServiceContext ToScada(const opcua::ServiceContext&);

opcua::ReadValueId ToOpcua(const scada::ReadValueId&);
scada::ReadValueId ToScada(const opcua::ReadValueId&);

opcua::WriteValue ToOpcua(const scada::WriteValue&);
scada::WriteValue ToScada(const opcua::WriteValue&);

opcua::BrowseDescription ToOpcua(const scada::BrowseDescription&);
scada::BrowseDescription ToScada(const opcua::BrowseDescription&);

opcua::ReferenceDescription ToOpcua(const scada::ReferenceDescription&);
scada::ReferenceDescription ToScada(const opcua::ReferenceDescription&);

opcua::BrowseResult ToOpcua(const scada::BrowseResult&);
scada::BrowseResult ToScada(const opcua::BrowseResult&);

opcua::RelativePathElement ToOpcua(const scada::RelativePathElement&);
scada::RelativePathElement ToScada(const opcua::RelativePathElement&);

opcua::BrowsePath ToOpcua(const scada::BrowsePath&);
scada::BrowsePath ToScada(const opcua::BrowsePath&);

opcua::BrowsePathTarget ToOpcua(const scada::BrowsePathTarget&);
scada::BrowsePathTarget ToScada(const opcua::BrowsePathTarget&);

opcua::BrowsePathResult ToOpcua(const scada::BrowsePathResult&);
scada::BrowsePathResult ToScada(const opcua::BrowsePathResult&);

opcua::NodeAttributes ToOpcua(const scada::NodeAttributes&);
scada::NodeAttributes ToScada(const opcua::NodeAttributes&);

opcua::AddNodesItem ToOpcua(const scada::AddNodesItem&);
scada::AddNodesItem ToScada(const opcua::AddNodesItem&);

opcua::AddNodesResult ToOpcua(const scada::AddNodesResult&);
scada::AddNodesResult ToScada(const opcua::AddNodesResult&);

opcua::DeleteNodesItem ToOpcua(const scada::DeleteNodesItem&);
scada::DeleteNodesItem ToScada(const opcua::DeleteNodesItem&);

opcua::AddReferencesItem ToOpcua(const scada::AddReferencesItem&);
scada::AddReferencesItem ToScada(const opcua::AddReferencesItem&);

opcua::DeleteReferencesItem ToOpcua(const scada::DeleteReferencesItem&);
scada::DeleteReferencesItem ToScada(const opcua::DeleteReferencesItem&);

opcua::DataChangeFilter ToOpcua(const scada::DataChangeFilter&);
scada::DataChangeFilter ToScada(const opcua::DataChangeFilter&);

opcua::AggregateFilter ToOpcua(const scada::AggregateFilter&);
scada::AggregateFilter ToScada(const opcua::AggregateFilter&);

opcua::EventFilter ToOpcua(const scada::EventFilter&);
scada::EventFilter ToScada(const opcua::EventFilter&);

opcua::MonitoringParameters ToOpcua(const scada::MonitoringParameters&);
scada::MonitoringParameters ToScada(const opcua::MonitoringParameters&);

opcua::Event ToOpcua(const scada::Event&);
scada::Event ToScada(const opcua::Event&);

// --- monitored item -----------------------------------------------------
opcua::MonitoredItemSubscriptionOptions ToOpcua(
    const scada::MonitoredItemSubscriptionOptions&);
scada::MonitoredItemSubscriptionOptions ToScada(
    const opcua::MonitoredItemSubscriptionOptions&);

opcua::MonitoredItemCreateRequest ToOpcua(
    const scada::MonitoredItemCreateRequest&);
scada::MonitoredItemCreateRequest ToScada(
    const opcua::MonitoredItemCreateRequest&);

opcua::MonitoredItemCreateResult ToOpcua(
    const scada::MonitoredItemCreateResult&);
scada::MonitoredItemCreateResult ToScada(
    const opcua::MonitoredItemCreateResult&);

// Maps a wire notification (data-change or projected event) back to a core
// notification. A wire MonitoredItemNotification becomes a core
// DataChangeNotification; a wire EventFieldList becomes a core
// EventNotification whose std::any payload is reassembled from the event
// fields. Consumers correlate by client_handle.
scada::MonitoredItemNotification ToScada(const opcua::ItemNotification&);

opcua::HistoryReadRawDetails ToOpcua(const scada::HistoryReadRawDetails&);
scada::HistoryReadRawDetails ToScada(const opcua::HistoryReadRawDetails&);

opcua::HistoryReadEventsDetails ToOpcua(const scada::HistoryReadEventsDetails&);
scada::HistoryReadEventsDetails ToScada(const opcua::HistoryReadEventsDetails&);

opcua::HistoryReadRawResult ToOpcua(const scada::HistoryReadRawResult&);
scada::HistoryReadRawResult ToScada(const opcua::HistoryReadRawResult&);

opcua::HistoryReadEventsResult ToOpcua(const scada::HistoryReadEventsResult&);
scada::HistoryReadEventsResult ToScada(const opcua::HistoryReadEventsResult&);

opcua::UpdateDataDetails ToOpcua(const scada::UpdateDataDetails&);
scada::UpdateDataDetails ToScada(const opcua::UpdateDataDetails&);

// --- session ------------------------------------------------------------
opcua::SessionSecuritySettings ToOpcua(const scada::SessionSecuritySettings&);
scada::SessionSecuritySettings ToScada(const opcua::SessionSecuritySettings&);

opcua::SessionConnectParams ToOpcua(const scada::SessionConnectParams&);
scada::SessionConnectParams ToScada(const opcua::SessionConnectParams&);

// --- authentication -----------------------------------------------------
inline opcua::AuthenticationResult ToOpcua(
    const scada::AuthenticationResult& v) {
  return {.user_id = ToOpcua(v.user_id),
          .user_rights = v.user_rights,
          .multi_sessions = v.multi_sessions};
}
inline scada::AuthenticationResult ToScada(
    const opcua::AuthenticationResult& v) {
  return {.user_id = ToScada(v.user_id),
          .user_rights = v.user_rights,
          .multi_sessions = v.multi_sessions};
}

}  // namespace opcua_bridge
