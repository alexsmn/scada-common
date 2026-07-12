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
using scada::opcua_bridge::ClientAttributeServiceAdapter;
using scada::opcua_bridge::ClientHistoryServiceAdapter;
using scada::opcua_bridge::ClientMethodServiceAdapter;
using scada::opcua_bridge::ClientMonitoredItemServiceAdapter;
using scada::opcua_bridge::ClientMonitoredItemSubscriptionAdapter;
using scada::opcua_bridge::ClientNodeManagementServiceAdapter;
using scada::opcua_bridge::ClientSessionServiceAdapter;
using scada::opcua_bridge::ClientViewServiceAdapter;

// server_adapters.h
using scada::opcua_bridge::AttributeServiceAdapter;
using scada::opcua_bridge::AuthenticatorAdapter;
using scada::opcua_bridge::HistoryServiceAdapter;
using scada::opcua_bridge::HistoryUpdateServiceAdapter;
using scada::opcua_bridge::MethodServiceAdapter;
using scada::opcua_bridge::MonitoredItemServiceAdapter;
using scada::opcua_bridge::MonitoredItemSubscriptionAdapter;
using scada::opcua_bridge::NodeManagementServiceAdapter;
using scada::opcua_bridge::ServerServiceAdapters;
using scada::opcua_bridge::ViewServiceAdapter;

// remote_history_service.h
using scada::opcua_bridge::RemoteHistoryService;
using scada::opcua_bridge::RemoteHistoryServiceConfig;

// conversion.h / service_conversion.h / vector_conversion.h: the whole
// ToOpcua/ToScada overload sets (exported once, after all GMF includes).
using scada::opcua_bridge::ToOpcua;
using scada::opcua_bridge::ToOpcuaVector;
using scada::opcua_bridge::ToScada;
using scada::opcua_bridge::ToScadaVector;

}  // namespace opcua_bridge
