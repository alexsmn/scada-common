#pragma once

#include "opcua_ws/opcua_ws_service_message.h"
#include "opcua_ws/opcua_ws_session_manager.h"

#include <variant>

namespace opcua_ws {

struct OpcUaWsServiceFault {
  scada::Status status{scada::StatusCode::Bad};
};

using OpcUaWsRequestBody =
    std::variant<OpcUaWsCreateSessionRequest,
                 OpcUaWsActivateSessionRequest,
                 OpcUaWsCloseSessionRequest,
                 ReadRequest,
                 WriteRequest,
                 BrowseRequest,
                 TranslateBrowsePathsRequest,
                 CallRequest,
                 HistoryReadRawRequest,
                 HistoryReadEventsRequest,
                 AddNodesRequest,
                 DeleteNodesRequest,
                 AddReferencesRequest,
                 DeleteReferencesRequest>;

using OpcUaWsResponseBody =
    std::variant<OpcUaWsCreateSessionResponse,
                 OpcUaWsActivateSessionResponse,
                 OpcUaWsCloseSessionResponse,
                 OpcUaWsServiceFault,
                 ReadResponse,
                 WriteResponse,
                 BrowseResponse,
                 TranslateBrowsePathsResponse,
                 CallResponse,
                 HistoryReadRawResponse,
                 HistoryReadEventsResponse,
                 AddNodesResponse,
                 DeleteNodesResponse,
                 AddReferencesResponse,
                 DeleteReferencesResponse>;

struct OpcUaWsRequestMessage {
  scada::UInt32 request_handle = 0;
  OpcUaWsRequestBody body;
};

struct OpcUaWsResponseMessage {
  scada::UInt32 request_handle = 0;
  OpcUaWsResponseBody body;
};

}  // namespace opcua_ws
