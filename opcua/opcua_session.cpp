#include "opcua/opcua_session.h"

#include "base/executor.h"
#include "opcua/opcua_conversion.h"
#include "opcua_subscription.h"
#include "scada/status_promise.h"

namespace {

OpcUa_ProxyStubConfiguration MakeProxyStubConfiguration() {
  OpcUa_ProxyStubConfiguration result = {};
  result.bProxyStub_Trace_Enabled =
      OpcUa_True;  // to deactivate Tracer set this variable Opc Ua False.
  result.uProxyStub_Trace_Level = OPCUA_TRACE_OUTPUT_LEVEL_ALL;
  result.iSerializer_MaxAlloc = -1;
  result.iSerializer_MaxStringLength = -1;
  result.iSerializer_MaxByteStringLength = -1;
  result.iSerializer_MaxArrayLength = -1;
  result.iSerializer_MaxMessageSize = -1;
  result.iSerializer_MaxRecursionDepth = -1;
  result.bSecureListener_ThreadPool_Enabled = OpcUa_False;
  result.iSecureListener_ThreadPool_MinThreads = -1;
  result.iSecureListener_ThreadPool_MaxThreads = -1;
  result.iSecureListener_ThreadPool_MaxJobs = -1;
  result.bSecureListener_ThreadPool_BlockOnAdd = OpcUa_True;
  result.uSecureListener_ThreadPool_Timeout = OPCUA_INFINITE;
  result.bTcpListener_ClientThreadsEnabled = OpcUa_False;
  result.iTcpListener_DefaultChunkSize = -1;
  result.iTcpConnection_DefaultChunkSize = -1;
  result.iTcpTransport_MaxMessageLength = -1;
  result.iTcpTransport_MaxChunkCount = -1;
  result.bTcpStream_ExpectWriteToBlock = OpcUa_True;
  return result;
}

}  // namespace

OpcUaSession::OpcUaSession(std::shared_ptr<Executor> executor)
    : executor_{std::move(executor)},
      proxy_stub_{platform_, MakeProxyStubConfiguration()},
      session_{channel_} {}

OpcUaSession::~OpcUaSession() {}

promise<void> OpcUaSession::Connect(const scada::SessionConnectParams& params) {
  connect_promise_ = promise<void>{};

  // TODO: Move to general layer.
  OpcUa_Trace_Initialize();
  OpcUa_Trace_ChangeTraceLevel(OPCUA_TRACE_OUTPUT_LEVEL_SYSTEM);

  opcua::client::ChannelContext context{
      const_cast<OpcUa_StringA>(params.connection_string.c_str()),
      &client_certificate_.get(),
      &client_private_key_.get(),
      &server_certificate_.get(),
      &pki_config_,
      requested_security_policy_uri.pass(),
      0,
      OpcUa_MessageSecurityMode_None,
      10000,
  };

  channel_.Connect(context, BindExecutor(executor_, weak_from_this(),
                                         [this](opcua::StatusCode status_code,
                                                OpcUa_Channel_Event event) {
                                           OnConnectionStateChanged(status_code,
                                                                    event);
                                         }));

  return connect_promise_;
}

promise<void> OpcUaSession::Reconnect() {
  return make_resolved_promise();
}

promise<void> OpcUaSession::Disconnect() {
  return scada::MakeRejectedStatusPromise(scada::StatusCode::Bad);
}

bool OpcUaSession::IsConnected(base::TimeDelta* ping_delay) const {
  if (ping_delay)
    *ping_delay = {};
  return true;
}

bool OpcUaSession::HasPrivilege(scada::Privilege privilege) const {
  return true;
}

scada::NodeId OpcUaSession::GetUserId() const {
  return {};
}

std::string OpcUaSession::GetHostName() const {
  return {};
}

boost::signals2::scoped_connection OpcUaSession::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& callback) {
  return boost::signals2::scoped_connection{};
}

void OpcUaSession::Browse(const scada::ServiceContext& context,
                          const std::vector<scada::BrowseDescription>& nodes,
                          const scada::BrowseCallback& callback) {
  auto ua_nodes =
      ConvertVector<OpcUa_BrowseDescription>(nodes.begin(), nodes.end());
  auto size = ua_nodes.size();

  session_.Browse({ua_nodes.data(), ua_nodes.size()},
                  [size, callback](opcua::StatusCode status_code,
                                   opcua::Span<OpcUa_BrowseResult> results) {
                    assert(!status_code || results.size() == size);
                    callback(ConvertStatusCode(status_code.code()),
                             ConvertVector<scada::BrowseResult>(results));
                  });
}

void OpcUaSession::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  assert(false);
}

OpcUaSubscription& OpcUaSession::GetDefaultSubscription() {
  if (!default_subscription_) {
    default_subscription_ = OpcUaSubscription::Create(OpcUaSubscriptionContext{
        .session_ = session_,
        .executor_ = executor_,
        .error_handler_ = BindCancelation(
            weak_from_this(), std::bind_front(&OpcUaSession::OnError, this))});
  }
  return *default_subscription_;
}

std::shared_ptr<scada::MonitoredItem> OpcUaSession::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params) {
  assert(!read_value_id.node_id.is_null());
  return GetDefaultSubscription().CreateMonitoredItem(read_value_id, params);
}

void OpcUaSession::Read(
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  assert(session_activated_);

  std::vector<OpcUa_ReadValueId> read_array(inputs->size());
  for (size_t i = 0; i < inputs->size(); ++i)
    Convert((*inputs)[i], read_array[i]);

  session_.Read({read_array.data(), read_array.size()},
                [callback](opcua::StatusCode status_code,
                           opcua::Span<OpcUa_DataValue> results) {
                  callback(ConvertStatusCode(status_code.code()),
                           ConvertVector<scada::DataValue>(results));
                });

  std::ranges::for_each(read_array, [](OpcUa_ReadValueId& value) {
    ::OpcUa_ReadValueId_Clear(&value);
  });
}

void OpcUaSession::Write(
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  assert(session_activated_);

  std::vector<OpcUa_WriteValue> write_array(inputs->size());
  for (size_t i = 0; i < inputs->size(); ++i)
    Convert(scada::WriteValue{(*inputs)[i]}, write_array[i]);

  session_.Write({write_array.data(), write_array.size()},
                 [callback](opcua::StatusCode status_code,
                            opcua::Span<OpcUa_StatusCode> results) {
                   callback(ConvertStatusCode(status_code.code()),
                            ConvertStatusCodeVector(results));
                 });

  std::ranges::for_each(write_array, [](OpcUa_WriteValue& value) {
    ::OpcUa_WriteValue_Clear(&value);
  });
}

void OpcUaSession::Call(const scada::NodeId& node_id,
                        const scada::NodeId& method_id,
                        const std::vector<scada::Variant>& arguments,
                        const scada::NodeId& user_id,
                        const scada::StatusCallback& callback) {
  opcua::CallMethodRequest request;
  Convert(node_id, request.ObjectId);
  Convert(method_id, request.MethodId);
  auto copied_arguments = arguments;
  auto opcua_arguments =
      ConvertFromVector<OpcUa_Variant>(std::move(copied_arguments));
  request.NoOfInputArguments = opcua_arguments.size();
  request.InputArguments = opcua_arguments.release();

  session_.Call({&request, 1},
                [callback](opcua::StatusCode status_code,
                           opcua::Span<const OpcUa_CallMethodResult> results) {
                  if (status_code.IsNotGood()) {
                    callback(ConvertStatusCode(status_code.code()));
                    return;
                  }
                  assert(results.size() == 1);
                  auto& result = results[0];
                  callback(ConvertStatusCode(result.StatusCode));
                });
}

void OpcUaSession::Reset() {
  if (default_subscription_)
    default_subscription_->Reset();
  session_.Reset();
  channel_.Reset();
  session_created_ = false;
  session_activated_ = false;
  connect_promise_ = promise<void>{};
}

void OpcUaSession::OnConnectionStateChanged(opcua::StatusCode status_code,
                                            OpcUa_Channel_Event event) {
  if (event != eOpcUa_Channel_Event_Connected) {
    OnError(ConvertStatusCode(status_code.code()));
    return;
  }

  if (!session_created_)
    CreateSession();
}

void OpcUaSession::CreateSession() {
  assert(!session_created_);
  assert(!session_activated_);

  opcua::CreateSessionRequest request;
  request.ClientDescription.ApplicationType = OpcUa_ApplicationType_Client;
  OpcUa_String_AttachReadOnly(&request.ClientDescription.ApplicationName.Text,
                              "TestClientName");
  OpcUa_String_AttachReadOnly(&request.ClientDescription.ApplicationUri,
                              "TestClientUri");
  OpcUa_String_AttachReadOnly(&request.ClientDescription.ProductUri,
                              "TestProductUri");

  session_.Create(
      request,
      BindExecutor(
          executor_, weak_from_this(), [this](opcua::StatusCode status_code) {
            OnCreateSessionResponse(ConvertStatusCode(status_code.code()));
          }));
}

void OpcUaSession::OnCreateSessionResponse(scada::Status&& status) {
  assert(!session_created_);
  assert(!session_activated_);

  if (!status) {
    OnError(std::move(status));
    return;
  }

  session_created_ = true;

  ActivateSession();
}

void OpcUaSession::ActivateSession() {
  assert(session_created_);
  assert(!session_activated_);

  session_.Activate(BindExecutor(
      executor_, weak_from_this(), [this](opcua::StatusCode status_code) {
        OnActivateSessionResponse(ConvertStatusCode(status_code.code()));
      }));
}

void OpcUaSession::OnActivateSessionResponse(scada::Status&& status) {
  assert(session_created_);
  assert(!session_activated_);

  if (!status) {
    OnError(std::move(status));
    return;
  }

  session_activated_ = true;

  connect_promise_.resolve();
}

void OpcUaSession::OnError(scada::Status&& status) {
  Reset();
  scada::CompleteStatusPromise(connect_promise_, std::move(status));
}

scada::SessionDebugger* OpcUaSession::GetSessionDebugger() {
  return nullptr;
}