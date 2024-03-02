#pragma once

#include "scada/attribute_service.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "scada/session_service.h"
#include "scada/status_promise.h"
#include "scada/view_service.h"

#include <opcuapp/client/channel.h>
#include <opcuapp/client/session.h>
#include <opcuapp/platform.h>
#include <opcuapp/proxy_stub.h>
#include <opcuapp/status_code.h>

class Executor;
class OpcUaSubscription;

class OpcUaSession final : public std::enable_shared_from_this<OpcUaSession>,
                           public scada::SessionService,
                           public scada::ViewService,
                           public scada::AttributeService,
                           public scada::MonitoredItemService,
                           public scada::MethodService {
 public:
  explicit OpcUaSession(std::shared_ptr<Executor> executor);
  virtual ~OpcUaSession();

  // scada::SessionService
  virtual scada::status_promise<void> Connect(
      const scada::SessionConnectParams& params) override;
  virtual scada::status_promise<void> Disconnect() override;
  virtual scada::status_promise<void> Reconnect() override;
  virtual bool IsConnected(
      base::TimeDelta* ping_delay = nullptr) const override;
  virtual bool HasPrivilege(scada::Privilege privilege) const override;
  virtual bool IsScada() const override { return false; }
  virtual scada::NodeId GetUserId() const override;
  virtual std::string GetHostName() const override;
  virtual boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override;
  virtual scada::SessionDebugger* GetSessionDebugger() override;

  // scada::ViewService
  virtual void Browse(
      const scada::ServiceContext& context,
      const std::vector<scada::BrowseDescription>& nodes,
      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

  // scada::MonitoredItemService
  virtual std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id,
      const scada::MonitoringParameters& params) override;

  // scada::AttributeService
  virtual void Read(
      const scada::ServiceContext& context,
      const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
      const scada::ReadCallback& callback) override;
  virtual void Write(
      const scada::ServiceContext& context,
      const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
      const scada::WriteCallback& callback) override;

  // scada::MethodService
  virtual void Call(const scada::NodeId& node_id,
                    const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) override;

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

  const std::shared_ptr<Executor> executor_;

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
  scada::status_promise<void> connect_promise_;

  // Created on demand.
  std::shared_ptr<OpcUaSubscription> default_subscription_;
};
