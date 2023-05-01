#include "vidicon_session.h"

#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/win/scoped_bstr.h"
#include "core/date_time.h"
#include "core/monitored_item.h"
#include "core/standard_node_ids.h"
#include "vidicon/vidicon_monitored_data_point.h"
#include "vidicon/vidicon_monitored_events.h"

#include <wrl/client.h>

namespace {

Microsoft::WRL::ComPtr<IClient> CreateTeleClient() {
  Microsoft::WRL::ComPtr<IClient> client;
  ::CoCreateInstance(__uuidof(Client), nullptr, CLSCTX_ALL,
                     IID_PPV_ARGS(&client));
  return client;
}

}  // namespace

VidiconSession::VidiconSession()
    : sync_attribute_service_{{address_space_}},
      sync_view_service_{{address_space_}} {}

VidiconSession::~VidiconSession() {}

promise<> VidiconSession::Connect(const std::string& connection_string,
                                  const scada::LocalizedText& user_name,
                                  const scada::LocalizedText& password,
                                  bool allow_remote_logoff) {
  teleclient_ = CreateTeleClient();
  if (!teleclient_)
    return MakeRejectedStatusPromise(scada::StatusCode::Bad);

  return make_resolved_promise();
}

promise<> VidiconSession::Reconnect() {
  return make_resolved_promise();
}

promise<> VidiconSession::Disconnect() {
  teleclient_.Reset();

  return make_resolved_promise();
}

bool VidiconSession::IsConnected(base::TimeDelta* ping_delay) const {
  return true;
}

bool VidiconSession::HasPrivilege(scada::Privilege privilege) const {
  return true;
}

scada::NodeId VidiconSession::GetUserId() const {
  return {};
}

std::string VidiconSession::GetHostName() const {
  return "Vidicon";
}

boost::signals2::scoped_connection VidiconSession::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& callback) {
  return boost::signals2::scoped_connection{};
}

void VidiconSession::HistoryReadRaw(
    const scada::HistoryReadRawDetails& details,
    const scada::HistoryReadRawCallback& callback) {
  callback({scada::StatusCode::Bad});
}

void VidiconSession::HistoryReadEvents(
    const scada::NodeId& node_id,
    base::Time from,
    base::Time to,
    const scada::EventFilter& filter,
    const scada::HistoryReadEventsCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

std::shared_ptr<scada::MonitoredItem> VidiconSession::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params) {
  if (read_value_id.attribute_id == scada::AttributeId::Value) {
    if (read_value_id.node_id.type() != scada::NodeIdType::Numeric)
      return nullptr;
    auto address =
        base::StringPrintf(L"CF:%d", read_value_id.node_id.numeric_id());
    Microsoft::WRL::ComPtr<IDataPoint> point;
    teleclient_->RequestPoint(base::win::ScopedBstr(address),
                              point.GetAddressOf());
    if (!point)
      return nullptr;
    return std::make_shared<VidiconMonitoredDataPoint>(std::move(point));
  }

  if (read_value_id.attribute_id == scada::AttributeId::EventNotifier) {
    assert(read_value_id.node_id == scada::id::Server);
    return std::make_shared<VidiconMonitoredEvents>();
  }

  return nullptr;
}

void VidiconSession::Read(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  attribute_service_.Read(context, inputs, callback);
}

void VidiconSession::Write(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void VidiconSession::Acknowledge(
    base::span<const scada::EventAcknowledgeId> acknowledge_ids,
    scada::DateTime acknowledge_time,
    const scada::NodeId& user_ids) {}

void VidiconSession::Call(const scada::NodeId& node_id,
                          const scada::NodeId& method_id,
                          const std::vector<scada::Variant>& arguments,
                          const scada::NodeId& user_id,
                          const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad);
}

void VidiconSession::AddNodes(const std::vector<scada::AddNodesItem>& inputs,
                              const scada::AddNodesCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void VidiconSession::DeleteNodes(
    const std::vector<scada::DeleteNodesItem>& inputs,
    const scada::DeleteNodesCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void VidiconSession::AddReferences(
    const std::vector<scada::AddReferencesItem>& inputs,
    const scada::AddReferencesCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void VidiconSession::DeleteReferences(
    const std::vector<scada::DeleteReferencesItem>& inputs,
    const scada::DeleteReferencesCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void VidiconSession::Browse(const std::vector<scada::BrowseDescription>& nodes,
                            const scada::BrowseCallback& callback) {
  view_service_.Browse(nodes, callback);
}

void VidiconSession::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  view_service_.TranslateBrowsePaths(browse_paths, callback);
}
