#pragma once

#include "scada/history_types.h"
#include "scada/method_service.h"
#include "scada/node_management_service.h"
#include "scada/status.h"

#include <variant>
#include <vector>

namespace opcua_ws {

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
    std::variant<CallRequest,
                 HistoryReadRawRequest,
                 HistoryReadEventsRequest,
                 AddNodesRequest,
                 DeleteNodesRequest,
                 AddReferencesRequest,
                 DeleteReferencesRequest>;

using OpcUaWsServiceResponse =
    std::variant<CallResponse,
                 HistoryReadRawResponse,
                 HistoryReadEventsResponse,
                 AddNodesResponse,
                 DeleteNodesResponse,
                 AddReferencesResponse,
                 DeleteReferencesResponse>;

}  // namespace opcua_ws
