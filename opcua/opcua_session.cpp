#include "opcua_session.h"

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "opcua/opcua_conversion.h"
#include "opcua_subscription.h"

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

static void OnBrowseResponse(const scada::BrowseCallback& callback,
                             scada::Status&& status,
                             std::vector<scada::BrowseResult> results) {
  callback(std::move(status), std::move(results));
}

static void OnReadResponse(const scada::ReadCallback& callback,
                           scada::Status&& status,
                           std::vector<scada::DataValue> results) {
  callback(std::move(status), std::move(results));
}

}  // namespace

OpcUaSession::OpcUaSession()
    : proxy_stub_{platform_, MakeProxyStubConfiguration()},
      session_{channel_} {}

OpcUaSession::~OpcUaSession() {}

void OpcUaSession::Connect(const std::string& connection_string,
                           const scada::LocalizedText& user_name,
                           const scada::LocalizedText& password,
                           bool allow_remote_logoff,
                           const scada::StatusCallback& callback) {
  connect_callback_ = std::move(callback);

  // TODO: Move to general layer.
  OpcUa_Trace_Initialize();
  OpcUa_Trace_ChangeTraceLevel(OPCUA_TRACE_OUTPUT_LEVEL_SYSTEM);

  opcua::client::ChannelContext context{
      const_cast<OpcUa_StringA>(connection_string.c_str()),
      &client_certificate_.get(),
      &client_private_key_,
      &server_certificate_.get(),
      &pki_config_,
      requested_security_policy_uri.pass(),
      0,
      OpcUa_MessageSecurityMode_None,
      10000,
  };

  auto runner = base::SequencedTaskRunnerHandle::Get();
  auto ref = shared_from_this();

  channel_.Connect(context, [runner, ref](opcua::StatusCode status_code,
                                          OpcUa_Channel_Event event) {
    runner->PostTask(FROM_HERE,
                     base::Bind(&OpcUaSession::OnConnectionStateChanged, ref,
                                status_code, event));
    return OpcUa_Good;
  });
}

void OpcUaSession::Reconnect() {}

void OpcUaSession::Disconnect(const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad);
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

void OpcUaSession::AddObserver(scada::SessionStateObserver& observer) {}

void OpcUaSession::RemoveObserver(scada::SessionStateObserver& observer) {}

void OpcUaSession::Browse(const std::vector<scada::BrowseDescription>& nodes,
                          const scada::BrowseCallback& callback) {
  auto runner = base::SequencedTaskRunnerHandle::Get();
  auto ua_nodes =
      ConvertVector<OpcUa_BrowseDescription>(nodes.begin(), nodes.end());
  auto size = ua_nodes.size();
  session_.Browse(
      {ua_nodes.data(), ua_nodes.size()},
      [runner, size, callback](opcua::StatusCode status_code,
                               opcua::Span<OpcUa_BrowseResult> results) {
        if (!status_code) {
          runner->PostTask(
              FROM_HERE,
              base::Bind(&OnBrowseResponse, callback,
                         ConvertStatusCode(status_code.code()),
                         base::Passed(std::vector<scada::BrowseResult>{})));
          return;
        }

        if (size != results.size()) {
          assert(false);
          runner->PostTask(
              FROM_HERE,
              base::Bind(&OnBrowseResponse, callback,
                         scada::StatusCode::Bad_CantParseString,
                         base::Passed(std::vector<scada::BrowseResult>{})));
          return;
        }

        runner->PostTask(
            FROM_HERE,
            base::Bind(
                &OnBrowseResponse, callback,
                ConvertStatusCode(status_code.code()),
                base::Passed(ConvertVector<scada::BrowseResult>(results))));
      });
}

void OpcUaSession::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathCallback& callback) {
  assert(false);
}

OpcUaSubscription& OpcUaSession::GetDefaultSubscription() {
  if (!default_subscription_) {
    std::weak_ptr<OpcUaSession> weak_ptr = shared_from_this();
    default_subscription_ = OpcUaSubscription::Create(OpcUaSubscriptionContext{
        session_,
        [this](scada::Status&& status) { OnError(std::move(status)); },
    });
  }
  return *default_subscription_;
}

std::unique_ptr<scada::MonitoredItem> OpcUaSession::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params) {
  assert(!read_value_id.node_id.is_null());
  return GetDefaultSubscription().CreateMonitoredItem(read_value_id, params);
}

void OpcUaSession::Read(const std::vector<scada::ReadValueId>& value_ids,
                        const scada::ReadCallback& callback) {
  assert(session_activated_);

  std::vector<OpcUa_ReadValueId> read_array(value_ids.size());
  for (size_t i = 0; i < value_ids.size(); ++i)
    Convert(value_ids[i], read_array[i]);

  auto runner = base::SequencedTaskRunnerHandle::Get();
  session_.Read(
      {read_array.data(), value_ids.size()},
      [runner, callback](opcua::StatusCode status_code,
                         opcua::Span<OpcUa_DataValue> results) {
        runner->PostTask(
            FROM_HERE,
            base::Bind(&OnReadResponse, callback,
                       ConvertStatusCode(status_code.code()),
                       base::Passed(ConvertVector<scada::DataValue>(results))));
      });

  std::for_each(
      read_array.begin(), read_array.end(),
      [](OpcUa_ReadValueId& value) { ::OpcUa_ReadValueId_Clear(&value); });
}

void OpcUaSession::Write(const std::vector<scada::WriteValueId>& value_ids,
                         const scada::NodeId& user_id,
                         const scada::WriteCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void OpcUaSession::Reset() {
  if (default_subscription_)
    default_subscription_->Reset();
  session_.Reset();
  channel_.Reset();
  session_created_ = false;
  session_activated_ = false;
  connect_callback_ = nullptr;
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

  auto ref = shared_from_this();
  auto runner = base::SequencedTaskRunnerHandle::Get();
  session_.Create(request, [runner, ref](opcua::StatusCode status_code) {
    runner->PostTask(FROM_HERE,
                     base::Bind(&OpcUaSession::OnCreateSessionResponse, ref,
                                ConvertStatusCode(status_code.code())));
  });
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

  auto ref = shared_from_this();
  auto runner = base::SequencedTaskRunnerHandle::Get();
  session_.Activate([runner, ref](opcua::StatusCode status_code) {
    runner->PostTask(FROM_HERE,
                     base::Bind(&OpcUaSession::OnActivateSessionResponse, ref,
                                ConvertStatusCode(status_code.code())));
  });
}

void OpcUaSession::OnActivateSessionResponse(scada::Status&& status) {
  assert(session_created_);
  assert(!session_activated_);

  if (!status) {
    OnError(std::move(status));
    return;
  }

  session_activated_ = true;

  if (const auto callback = std::move(connect_callback_)) {
    connect_callback_ = nullptr;
    callback(scada::StatusCode::Good);
  }
}

void OpcUaSession::OnError(scada::Status&& status) {
  const auto callback = std::move(connect_callback_);
  Reset();
  if (callback)
    callback(std::move(status));
}
