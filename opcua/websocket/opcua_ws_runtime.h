#pragma once

#include "opcua/opcua_runtime.h"
#include "opcua/websocket/opcua_ws_message.h"
#include "opcua/websocket/opcua_ws_service_message.h"
#include "opcua/websocket/opcua_ws_session_manager.h"

namespace opcua_ws {

using OpcUaWsConnectionState = opcua::OpcUaConnectionState;
using OpcUaWsRuntimeContext = opcua::OpcUaRuntimeContext;
using OpcUaWsRuntime = opcua::OpcUaRuntime;

}  // namespace opcua_ws
