// scada.opcua_bridge — named C++20 module facade over the
// common/opcua_bridge headers (the scada:: <-> opcua:: boundary adapter).
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md). `export import scada.core;` mirrors the PUBLIC
// link on scada_core. The opcua:: type universe (third_party/opcuapp) is
// third-party: its names are pulled into the GMF by these headers but are
// deliberately not exported - TUs that name opcua:: types include the
// opcuapp headers textually alongside the import.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "opcua_bridge/client_adapters.h"
#include "opcua_bridge/conversion.h"
#include "opcua_bridge/remote_history_service.h"
#include "opcua_bridge/server_adapters.h"
#include "opcua_bridge/service_conversion.h"
#include "opcua_bridge/vector_conversion.h"

export module scada.opcua_bridge;

export import scada.core;

export namespace opcua_bridge {

// client_adapters.h
using opcua_bridge::ClientAttributeServiceAdapter;
using opcua_bridge::ClientHistoryServiceAdapter;
using opcua_bridge::ClientMethodServiceAdapter;
using opcua_bridge::ClientMonitoredItemServiceAdapter;
using opcua_bridge::ClientMonitoredItemSubscriptionAdapter;
using opcua_bridge::ClientNodeManagementServiceAdapter;
using opcua_bridge::ClientSessionServiceAdapter;
using opcua_bridge::ClientViewServiceAdapter;

// server_adapters.h
using opcua_bridge::AttributeServiceAdapter;
using opcua_bridge::AuthenticatorAdapter;
using opcua_bridge::HistoryServiceAdapter;
using opcua_bridge::HistoryUpdateServiceAdapter;
using opcua_bridge::MethodServiceAdapter;
using opcua_bridge::MonitoredItemServiceAdapter;
using opcua_bridge::MonitoredItemSubscriptionAdapter;
using opcua_bridge::NodeManagementServiceAdapter;
using opcua_bridge::ServerServiceAdapters;
using opcua_bridge::ViewServiceAdapter;

// remote_history_service.h
using opcua_bridge::RemoteHistoryService;
using opcua_bridge::RemoteHistoryServiceConfig;

// conversion.h / service_conversion.h / vector_conversion.h: the whole
// ToOpcua/ToScada overload sets (exported once, after all GMF includes).
using opcua_bridge::ToOpcua;
using opcua_bridge::ToOpcuaVector;
using opcua_bridge::ToScada;
using opcua_bridge::ToScadaVector;

}  // namespace opcua_bridge
