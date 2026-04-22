#pragma once

#include "opcua_ws/opcua_ws_message.h"
#include "opcua_ws/opcua_ws_service_message.h"
#include "opcua_ws/opcua_ws_session.h"
#include "opcua_ws/opcua_ws_session_manager.h"
#include "opcua/opcua_runtime.h"

namespace opcua_ws {

using OpcUaWsConnectionState = opcua::OpcUaConnectionState;
using OpcUaWsRuntimeContext = opcua::OpcUaRuntimeContext;
using OpcUaWsRuntime = opcua::OpcUaRuntime;

}  // namespace opcua_ws
