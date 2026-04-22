#pragma once

#include "opcua_ws/opcua_ws_message.h"
#include "opcua_ws/opcua_ws_service_message.h"
#include "opcua/opcua_server_session.h"

namespace opcua_ws {

using OpcUaWsSessionContext = opcua::OpcUaSessionContext;
using OpcUaWsSession = opcua::OpcUaSession;

}  // namespace opcua_ws
