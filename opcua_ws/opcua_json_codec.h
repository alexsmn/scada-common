#pragma once

#include "opcua_ws/opcua_ws_message.h"

#include <boost/json/value.hpp>

namespace opcua_ws {

boost::json::value EncodeJson(const OpcUaWsServiceRequest& request);
boost::json::value EncodeJson(const OpcUaWsServiceResponse& response);
boost::json::value EncodeJson(const OpcUaWsRequestMessage& request);
boost::json::value EncodeJson(const OpcUaWsResponseMessage& response);

OpcUaWsServiceRequest DecodeServiceRequest(const boost::json::value& json);
OpcUaWsServiceResponse DecodeServiceResponse(const boost::json::value& json);
OpcUaWsRequestMessage DecodeRequestMessage(const boost::json::value& json);
OpcUaWsResponseMessage DecodeResponseMessage(const boost::json::value& json);

}  // namespace opcua_ws
