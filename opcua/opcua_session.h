#pragma once

#include "core/attribute_service.h"
#include "core/monitored_item_service.h"
#include "core/session_service.h"
#include "core/view_service.h"

#include <opcuapp/client/channel.h>
#include <opcuapp/client/session.h>
#include <opcuapp/platform.h>
#include <opcuapp/proxy_stub.h>
#include <opcuapp/status_code.h>
#include <boost/asio/io_context_strand.hpp>

namespace boost::asio {
class io_context;
}

class OpcUaSubscription;

class OpcUaSession : public std::enable_shared_from_this<OpcUaSession>,
                     public scada::SessionService,
                     public scada::ViewService,
                     public scada::AttributeService,
                     public scada::MonitoredItemService {
 public:
  explicit OpcUaSession(boost::asio::io_context& io_context);
  virtual ~OpcUaSession();

  // scada::SessionService
  virtual void Connect(const std::string& connection_string,
                       const scada::LocalizedText& user_name,
                       const scada::LocalizedText& password,
                       bool allow_remote_logoff,
                       const scada::StatusCallback& callback) override;
  virtual void Reconnect() override;
  virtual void Disconnect(const scada::StatusCallback& callback) override;
  virtual bool IsConnected(
      base::TimeDelta* ping_delay = nullptr) const override;
  virtual bool HasPrivilege(scada::Privilege privilege) const override;
  virtual bool IsScada() const override { return false; }
  virtual scada::NodeId GetUserId() const override;
  virtual std::string GetHostName() const override;
  virtual void AddObserver(scada::SessionStateObserver& observer) override;
  virtual void RemoveObserver(scada::SessionStateObserver& observer) override;

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& nodes,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

  // scada::MonitoredItemService
  virtual std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id,
      const scada::MonitoringParameters& params) override;

  // scada::AttributeService
  virtual void Read(const std::vector<scada::ReadValueId>& value_ids,
                    const scada::ReadCallback& callback) override;
  virtual void Write(const std::vector<scada::WriteValueId>& value_ids,
                     const scada::NodeId& user_id,
                     const scada::WriteCallback& callback) override;

 private:
  void Reset();

  void CreateSession();
  void OnCreateSessionResponse(scada::Status&& status);

  void ActivateSession();
  void OnActivateSessionResponse(scada::Status&& status);

  void OnConnectionStateChanged(opcua::StatusCode status_code,
                                OpcUa_Channel_Event event);
  void OnError(scada::Status&& status);

  OpcUaSubscription& GetDefaultSubscription();

  boost::asio::io_context& io_context_;
  boost::asio::io_context::strand executor_{io_context_};

  opcua::Platform platform_;
  opcua::ProxyStub proxy_stub_;

  const opcua::ByteString client_certificate_;
  const opcua::Key client_private_key_;
  const opcua::ByteString server_certificate_;
  const OpcUa_P_OpenSSL_CertificateStore_Config pki_config_{OpcUa_NO_PKI};
  const opcua::String requested_security_policy_uri{OpcUa_SecurityPolicy_None};
  opcua::client::Channel channel_{OpcUa_Channel_SerializerType_Binary};

  opcua::client::Session session_;

  bool session_created_ = false;
  bool session_activated_ = false;
  scada::StatusCallback connect_callback_;

  // Created on demand.
  std::shared_ptr<OpcUaSubscription> default_subscription_;
};
