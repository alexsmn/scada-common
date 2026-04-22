#pragma once

#include "opcua/opcua_service_message.h"

namespace opcua_ws {

using opcua::AddNodesRequest;
using opcua::AddNodesResponse;
using opcua::AddReferencesRequest;
using opcua::AddReferencesResponse;
using opcua::BrowseNextRequest;
using opcua::BrowseNextResponse;
using opcua::BrowseRequest;
using opcua::BrowseResponse;
using opcua::CallMethodRequest;
using opcua::CallMethodResult;
using opcua::CallRequest;
using opcua::CallResponse;
using opcua::DeleteNodesRequest;
using opcua::DeleteNodesResponse;
using opcua::DeleteReferencesRequest;
using opcua::DeleteReferencesResponse;
using opcua::HistoryReadEventsRequest;
using opcua::HistoryReadEventsResponse;
using opcua::HistoryReadRawRequest;
using opcua::HistoryReadRawResponse;
using opcua::OpcUaServiceRequest;
using opcua::OpcUaServiceResponse;
using opcua::ReadRequest;
using opcua::ReadResponse;
using opcua::TranslateBrowsePathsRequest;
using opcua::TranslateBrowsePathsResponse;
using opcua::WriteRequest;
using opcua::WriteResponse;

using OpcUaWsServiceRequest = opcua::OpcUaServiceRequest;
using OpcUaWsServiceResponse = opcua::OpcUaServiceResponse;

}  // namespace opcua_ws
