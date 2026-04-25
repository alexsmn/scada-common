#pragma once

#include "opcua/message.h"
#include "opcua/service_message.h"

#include <boost/json/value.hpp>

namespace opcua {

boost::json::value EncodeJson(const opcua::OpcUaServiceRequest& request);
boost::json::value EncodeJson(const opcua::OpcUaServiceResponse& response);
boost::json::value EncodeJson(const opcua::OpcUaRequestMessage& request);
boost::json::value EncodeJson(const opcua::OpcUaResponseMessage& response);

opcua::OpcUaServiceRequest DecodeServiceRequest(const boost::json::value& json);
opcua::OpcUaServiceResponse DecodeServiceResponse(const boost::json::value& json);
opcua::OpcUaRequestMessage DecodeRequestMessage(const boost::json::value& json);
opcua::OpcUaResponseMessage DecodeResponseMessage(const boost::json::value& json);

}  // namespace opcua
