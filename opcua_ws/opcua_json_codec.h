#pragma once

#include "opcua_ws/opcua_ws_service_message.h"

#include <boost/json/value.hpp>

namespace opcua_ws {

boost::json::value EncodeJson(const OpcUaWsServiceRequest& request);
boost::json::value EncodeJson(const OpcUaWsServiceResponse& response);

OpcUaWsServiceRequest DecodeServiceRequest(const boost::json::value& json);
OpcUaWsServiceResponse DecodeServiceResponse(const boost::json::value& json);

}  // namespace opcua_ws
