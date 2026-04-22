#pragma once

#include "opcua/opcua_server_session_manager.h"

namespace opcua_ws {

using OpcUaWsCreateSessionRequest = opcua::OpcUaCreateSessionRequest;
using OpcUaWsCreateSessionResponse = opcua::OpcUaCreateSessionResponse;
using OpcUaWsActivateSessionRequest = opcua::OpcUaActivateSessionRequest;
using OpcUaWsActivateSessionResponse = opcua::OpcUaActivateSessionResponse;
using OpcUaWsCloseSessionRequest = opcua::OpcUaCloseSessionRequest;
using OpcUaWsCloseSessionResponse = opcua::OpcUaCloseSessionResponse;
using OpcUaWsSessionLookupResult = opcua::OpcUaSessionLookupResult;
using OpcUaWsSessionManagerContext = opcua::OpcUaSessionManagerContext;
using OpcUaWsSessionManager = opcua::OpcUaSessionManager;

}  // namespace opcua_ws
