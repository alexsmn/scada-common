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

std::optional<OpcUaBinaryDecodedRequest> DecodeOpcUaBinaryServiceRequest(
    const std::vector<char>& payload);

}  // namespace opcua
