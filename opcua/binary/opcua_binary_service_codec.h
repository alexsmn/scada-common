#pragma once

#include "opcua/opcua_message.h"

#include <optional>
#include <span>
#include <vector>

namespace opcua {

struct OpcUaBinaryServiceRequestHeader {
  scada::NodeId authentication_token;
  std::uint32_t request_handle = 0;
};

struct OpcUaBinaryDecodedRequest {
  OpcUaBinaryServiceRequestHeader header;
  OpcUaRequestBody body;
  std::vector<std::vector<std::string>> history_event_field_paths;
};

std::optional<std::vector<char>> EncodeOpcUaBinaryServiceRequest(
    const OpcUaBinaryServiceRequestHeader& header,
    const OpcUaRequestBody& request);

std::optional<OpcUaBinaryDecodedRequest> DecodeOpcUaBinaryServiceRequest(
    const std::vector<char>& payload);

std::optional<std::vector<char>> EncodeOpcUaBinaryServiceResponse(
    std::uint32_t request_handle,
    const OpcUaResponseBody& response);

std::optional<std::vector<char>> EncodeOpcUaBinaryHistoryReadEventsResponse(
    std::uint32_t request_handle,
    const HistoryReadEventsResponse& response,
    std::span<const std::vector<std::string>> field_paths);

}  // namespace opcua
