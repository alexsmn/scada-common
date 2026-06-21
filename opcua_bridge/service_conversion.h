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

#include "opcua/scada/attribute_service.h"
#include "opcua/scada/authentication.h"
#include "opcua/scada/event.h"
#include "opcua/scada/history_types.h"
#include "opcua/scada/monitored_item.h"
#include "opcua/scada/monitoring_parameters.h"
#include "opcua/scada/node_attributes.h"
#include "opcua/scada/node_management_service.h"
#include "opcua/scada/privileges.h"
#include "opcua/scada/read_value_id.h"
#include "opcua/scada/service_context.h"
#include "opcua/scada/session_service.h"
#include "opcua/scada/view_service.h"
#include "opcua/scada/write_flags.h"

namespace opcua_bridge {

// --- enums (identical underlying values) --------------------------------
inline opcua::scada::AttributeId ToOpcua(scada::AttributeId v) {
  return static_cast<opcua::scada::AttributeId>(v);
}
inline scada::AttributeId ToScada(opcua::scada::AttributeId v) {
  return static_cast<scada::AttributeId>(v);
}
inline opcua::scada::NodeClass ToOpcua(scada::NodeClass v) {
  return static_cast<opcua::scada::NodeClass>(v);
}
inline scada::NodeClass ToScada(opcua::scada::NodeClass v) {
  return static_cast<scada::NodeClass>(v);
}
inline opcua::scada::BrowseDirection ToOpcua(scada::BrowseDirection v) {
  return static_cast<opcua::scada::BrowseDirection>(v);
}
inline scada::BrowseDirection ToScada(opcua::scada::BrowseDirection v) {
  return static_cast<scada::BrowseDirection>(v);
}
inline opcua::scada::Privilege ToOpcua(scada::Privilege v) {
  return static_cast<opcua::scada::Privilege>(v);
}
inline scada::Privilege ToScada(opcua::scada::Privilege v) {
  return static_cast<scada::Privilege>(v);
}

// --- Duration (base::TimeDelta vs opcua::base::TimeDelta) ----------------
inline opcua::scada::Duration ToOpcua(scada::Duration d) {
  return opcua::base::TimeDelta::FromInternalValue(d.ToInternalValue());
}
inline scada::Duration ToScada(opcua::scada::Duration d) {
  return base::TimeDelta::FromInternalValue(d.ToInternalValue());
}

// --- WriteFlags ---------------------------------------------------------
inline opcua::scada::WriteFlags ToOpcua(scada::WriteFlags v) {
  return opcua::scada::WriteFlags{v.raw()};
}
inline scada::WriteFlags ToScada(opcua::scada::WriteFlags v) {
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
opcua::scada::ServiceContext ToOpcua(const scada::ServiceContext&);
scada::ServiceContext ToScada(const opcua::scada::ServiceContext&);

opcua::scada::ReadValueId ToOpcua(const scada::ReadValueId&);
scada::ReadValueId ToScada(const opcua::scada::ReadValueId&);

opcua::scada::WriteValue ToOpcua(const scada::WriteValue&);
scada::WriteValue ToScada(const opcua::scada::WriteValue&);

opcua::scada::BrowseDescription ToOpcua(const scada::BrowseDescription&);
scada::BrowseDescription ToScada(const opcua::scada::BrowseDescription&);

opcua::scada::ReferenceDescription ToOpcua(const scada::ReferenceDescription&);
scada::ReferenceDescription ToScada(const opcua::scada::ReferenceDescription&);

opcua::scada::BrowseResult ToOpcua(const scada::BrowseResult&);
scada::BrowseResult ToScada(const opcua::scada::BrowseResult&);

opcua::scada::RelativePathElement ToOpcua(const scada::RelativePathElement&);
scada::RelativePathElement ToScada(const opcua::scada::RelativePathElement&);

opcua::scada::BrowsePath ToOpcua(const scada::BrowsePath&);
scada::BrowsePath ToScada(const opcua::scada::BrowsePath&);

opcua::scada::BrowsePathTarget ToOpcua(const scada::BrowsePathTarget&);
scada::BrowsePathTarget ToScada(const opcua::scada::BrowsePathTarget&);

opcua::scada::BrowsePathResult ToOpcua(const scada::BrowsePathResult&);
scada::BrowsePathResult ToScada(const opcua::scada::BrowsePathResult&);

opcua::scada::NodeAttributes ToOpcua(const scada::NodeAttributes&);
scada::NodeAttributes ToScada(const opcua::scada::NodeAttributes&);

opcua::scada::AddNodesItem ToOpcua(const scada::AddNodesItem&);
scada::AddNodesItem ToScada(const opcua::scada::AddNodesItem&);

opcua::scada::AddNodesResult ToOpcua(const scada::AddNodesResult&);
scada::AddNodesResult ToScada(const opcua::scada::AddNodesResult&);

opcua::scada::DeleteNodesItem ToOpcua(const scada::DeleteNodesItem&);
scada::DeleteNodesItem ToScada(const opcua::scada::DeleteNodesItem&);

opcua::scada::AddReferencesItem ToOpcua(const scada::AddReferencesItem&);
scada::AddReferencesItem ToScada(const opcua::scada::AddReferencesItem&);

opcua::scada::DeleteReferencesItem ToOpcua(const scada::DeleteReferencesItem&);
scada::DeleteReferencesItem ToScada(const opcua::scada::DeleteReferencesItem&);

opcua::scada::DataChangeFilter ToOpcua(const scada::DataChangeFilter&);
scada::DataChangeFilter ToScada(const opcua::scada::DataChangeFilter&);

opcua::scada::AggregateFilter ToOpcua(const scada::AggregateFilter&);
scada::AggregateFilter ToScada(const opcua::scada::AggregateFilter&);

opcua::scada::EventFilter ToOpcua(const scada::EventFilter&);
scada::EventFilter ToScada(const opcua::scada::EventFilter&);

opcua::scada::MonitoringParameters ToOpcua(const scada::MonitoringParameters&);
scada::MonitoringParameters ToScada(const opcua::scada::MonitoringParameters&);

opcua::scada::Event ToOpcua(const scada::Event&);
scada::Event ToScada(const opcua::scada::Event&);

// --- monitored item -----------------------------------------------------
opcua::scada::MonitoredItemSubscriptionOptions ToOpcua(
    const scada::MonitoredItemSubscriptionOptions&);
scada::MonitoredItemSubscriptionOptions ToScada(
    const opcua::scada::MonitoredItemSubscriptionOptions&);

opcua::scada::MonitoredItemCreateRequest ToOpcua(
    const scada::MonitoredItemCreateRequest&);
scada::MonitoredItemCreateRequest ToScada(
    const opcua::scada::MonitoredItemCreateRequest&);

opcua::scada::MonitoredItemCreateResult ToOpcua(
    const scada::MonitoredItemCreateResult&);
scada::MonitoredItemCreateResult ToScada(
    const opcua::scada::MonitoredItemCreateResult&);

opcua::scada::MonitoredItemNotification ToOpcua(
    const scada::MonitoredItemNotification&);
scada::MonitoredItemNotification ToScada(
    const opcua::scada::MonitoredItemNotification&);

opcua::scada::HistoryReadRawDetails ToOpcua(const scada::HistoryReadRawDetails&);
scada::HistoryReadRawDetails ToScada(const opcua::scada::HistoryReadRawDetails&);

opcua::scada::HistoryReadEventsDetails ToOpcua(
    const scada::HistoryReadEventsDetails&);
scada::HistoryReadEventsDetails ToScada(
    const opcua::scada::HistoryReadEventsDetails&);

opcua::scada::HistoryReadRawResult ToOpcua(const scada::HistoryReadRawResult&);
scada::HistoryReadRawResult ToScada(const opcua::scada::HistoryReadRawResult&);

opcua::scada::HistoryReadEventsResult ToOpcua(
    const scada::HistoryReadEventsResult&);
scada::HistoryReadEventsResult ToScada(
    const opcua::scada::HistoryReadEventsResult&);

// --- session ------------------------------------------------------------
opcua::scada::SessionSecuritySettings ToOpcua(
    const scada::SessionSecuritySettings&);
scada::SessionSecuritySettings ToScada(
    const opcua::scada::SessionSecuritySettings&);

opcua::scada::SessionConnectParams ToOpcua(const scada::SessionConnectParams&);
scada::SessionConnectParams ToScada(const opcua::scada::SessionConnectParams&);

// --- authentication -----------------------------------------------------
inline opcua::scada::AuthenticationResult ToOpcua(
    const scada::AuthenticationResult& v) {
  return {.user_id = ToOpcua(v.user_id),
          .user_rights = v.user_rights,
          .multi_sessions = v.multi_sessions};
}
inline scada::AuthenticationResult ToScada(
    const opcua::scada::AuthenticationResult& v) {
  return {.user_id = ToScada(v.user_id),
          .user_rights = v.user_rights,
          .multi_sessions = v.multi_sessions};
}

}  // namespace opcua_bridge
