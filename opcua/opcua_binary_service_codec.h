#pragma once

#include "opcua/opcua_binary_message.h"

#include <optional>
#include <vector>

namespace opcua {

struct OpcUaBinaryServiceRequestHeader {
  scada::NodeId authentication_token;
  std::uint32_t request_handle = 0;
};

struct OpcUaBinaryDecodedRequest {
  OpcUaBinaryServiceRequestHeader header;
  OpcUaBinaryRequestBody body;
};

std::optional<std::vector<char>> EncodeOpcUaBinaryServiceRequest(
    const OpcUaBinaryServiceRequestHeader& header,
    const OpcUaBinaryRequestBody& request);

std::optional<OpcUaBinaryDecodedRequest> DecodeOpcUaBinaryServiceRequest(
    const std::vector<char>& payload);

std::optional<std::vector<char>> EncodeOpcUaBinaryServiceResponse(
    std::uint32_t request_handle,
    const OpcUaBinaryResponseBody& response);

}  // namespace opcua
