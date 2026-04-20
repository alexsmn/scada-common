#pragma once

#include "scada/attribute_service.h"
#include "scada/history_types.h"
#include "scada/method_service.h"
#include "scada/node_management_service.h"
#include "scada/status.h"
#include "scada/view_service.h"

#include <variant>
#include <vector>

namespace opcua_ws {

struct ReadRequest {
  std::vector<scada::ReadValueId> inputs;
};

struct ReadResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::DataValue> results;
};

struct WriteRequest {
  std::vector<scada::WriteValue> inputs;
};

struct WriteResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct BrowseRequest {
  std::vector<scada::BrowseDescription> inputs;
};

struct BrowseResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::BrowseResult> results;
};

struct TranslateBrowsePathsRequest {
  std::vector<scada::BrowsePath> inputs;
};

struct TranslateBrowsePathsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::BrowsePathResult> results;
};

struct CallMethodRequest {
  scada::NodeId object_id;
  scada::NodeId method_id;
  std::vector<scada::Variant> arguments;
};

struct CallMethodResult {
  scada::Status status{scada::StatusCode::Good};
};

struct CallRequest {
  std::vector<CallMethodRequest> methods;
};

struct CallResponse {
  std::vector<CallMethodResult> results;
};

struct HistoryReadRawRequest {
  scada::HistoryReadRawDetails details;
};

struct HistoryReadRawResponse {
  scada::HistoryReadRawResult result;
};

struct HistoryReadEventsRequest {
  scada::HistoryReadEventsDetails details;
};

struct HistoryReadEventsResponse {
  scada::HistoryReadEventsResult result;
};

struct AddNodesRequest {
  std::vector<scada::AddNodesItem> items;
};

struct AddNodesResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::AddNodesResult> results;
};

struct DeleteNodesRequest {
  std::vector<scada::DeleteNodesItem> items;
};

struct DeleteNodesResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct AddReferencesRequest {
  std::vector<scada::AddReferencesItem> items;
};

struct AddReferencesResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct DeleteReferencesRequest {
  std::vector<scada::DeleteReferencesItem> items;
};

struct DeleteReferencesResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

using OpcUaWsServiceRequest =
    std::variant<ReadRequest,
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

using OpcUaWsServiceResponse =
    std::variant<ReadResponse,
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

}  // namespace opcua_ws
