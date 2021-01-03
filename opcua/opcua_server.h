#pragma once

#include "base/boost_log.h"
#include "core/configuration_types.h"
#include "core/node_id.h"

#include <map>
#include <opcua_serverapi.h>
#include <opcuapp/basic_types.h>
#include <opcuapp/platform.h>
#include <opcuapp/proxy_stub.h>
#include <opcuapp/server/endpoint.h>
#include <opcuapp/structs.h>
#include <opcuapp/vector.h>
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
  std::string url_;
  std::vector<char> server_certificate_data_;
  std::vector<char> server_private_key_data_;
  opcua::UInt32 trace_level_ = OPCUA_TRACE_OUTPUT_LEVEL_WARNING;
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
  void TranslateBrowsePaths(
      OpcUa_TranslateBrowsePathsToNodeIdsRequest& request,
      const opcua::server::TranslateBrowsePathsToNodeIdsCallback& callback);
  opcua::server::CreateMonitoredItemResult CreateMonitoredItem(
      opcua::ReadValueId&& read_value_id,
      opcua::MonitoringParameters&& params);

  opcua::ProxyStubConfiguration MakeProxyStubConfiguration();

  BoostLogger logger_{LOG_NAME("OpcUaServer")};

  opcua::Platform platform_;
  opcua::ProxyStub proxy_stub_;

  const opcua::ByteString server_certificate_{server_certificate_data_.data(),
                                              server_certificate_data_.size()};

  const opcua::Key server_private_key_ =
      server_private_key_data_.empty()
          ? opcua::Key{}
          : opcua::Key{OpcUa_Crypto_KeyType_Rsa_Private,
                       opcua::ByteString{server_private_key_data_.data(),
                                         server_private_key_data_.size()}};

  const OpcUa_P_OpenSSL_CertificateStore_Config pki_config_{
      OpcUa_NO_PKI, OpcUa_Null, OpcUa_Null, OpcUa_Null, 0, OpcUa_Null};

  const opcua::server::Endpoint::SecurityPolicyConfiguration security_policy_;

  opcua::server::Endpoint endpoint_{OpcUa_Endpoint_SerializerType_Binary};
};

opcua::UInt32 ParseTraceLevel(std::string_view str);
