#pragma once

#include "core/configuration_types.h"
#include "core/node_id.h"

#include <opcua_serverapi.h>
#include <opcuapp/basic_types.h>
#include <opcuapp/platform.h>
#include <opcuapp/proxy_stub.h>
#include <opcuapp/server/endpoint.h>
#include <opcuapp/structs.h>
#include <opcuapp/vector.h>
#include <map>
#include <vector>

namespace scada {
class AttributeService;
class MonitoredItemService;
class ViewService;
}  // namespace scada

struct OpcUaServerContext {
  scada::AttributeService& attribute_service_;
  scada::ViewService& view_service_;
  scada::MonitoredItemService& monitored_item_service_;
  const std::string url_;
};

class OpcUaServer : private OpcUaServerContext {
 public:
  explicit OpcUaServer(OpcUaServerContext&& context);
  ~OpcUaServer();

 private:
  void Read(OpcUa_ReadRequest& request,
            const opcua::server::ReadCallback& callback);
  void Browse(OpcUa_BrowseRequest& request,
              const opcua::server::BrowseCallback& callback);
  opcua::server::CreateMonitoredItemResult CreateMonitoredItem(
      opcua::ReadValueId& read_value_id);

  opcua::Platform platform_;
  opcua::ProxyStub proxy_stub_;

  const opcua::ByteString server_certificate_;
  const OpcUa_Key server_private_key_{OpcUa_Crypto_KeyType_Invalid,
                                      {0, (OpcUa_Byte*)""}};
  const OpcUa_P_OpenSSL_CertificateStore_Config pki_config_{
      OpcUa_NO_PKI, OpcUa_Null, OpcUa_Null, OpcUa_Null, 0, OpcUa_Null};
  const opcua::server::Endpoint::SecurityPolicyConfiguration security_policy_;
  opcua::server::Endpoint endpoint_{OpcUa_Endpoint_SerializerType_Binary};
};
