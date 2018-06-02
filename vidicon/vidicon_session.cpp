#include "vidicon_session.h"

#include "base/strings/stringprintf.h"
#include "base/win/scoped_bstr.h"
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

VidiconSession::VidiconSession() : teleclient_(CreateTeleClient()) {}

VidiconSession::~VidiconSession() {}

void VidiconSession::Connect(const std::string& connection_string,
                             const scada::LocalizedText& user_name,
                             const scada::LocalizedText& password,
                             bool allow_remote_logoff,
                             const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Good);
}

void VidiconSession::Reconnect() {}

void VidiconSession::Disconnect(const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Good);
}

bool VidiconSession::IsConnected(base::TimeDelta* ping_delay) const {
  return true;
}

bool VidiconSession::HasPrivilege(scada::Privilege privilege) const {
  return false;
}

scada::NodeId VidiconSession::GetUserId() const {
  return {};
}

std::string VidiconSession::GetHostName() const {
  return "Vidicon";
}

void VidiconSession::AddObserver(scada::SessionStateObserver& observer) {}

void VidiconSession::RemoveObserver(scada::SessionStateObserver& observer) {}

void VidiconSession::HistoryReadRaw(
    const scada::NodeId& node_id,
    base::Time from,
    base::Time to,
    const scada::HistoryReadRawCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void VidiconSession::HistoryReadEvents(
    const scada::NodeId& node_id,
    base::Time from,
    base::Time to,
    const scada::EventFilter& filter,
    const scada::HistoryReadEventsCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

std::unique_ptr<scada::MonitoredItem> VidiconSession::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id) {
  if (read_value_id.attribute_id == scada::AttributeId::Value) {
    if (read_value_id.node_id.type() != scada::NodeIdType::Numeric)
      return nullptr;
    auto address =
        base::StringPrintf(L"CF:%d", read_value_id.node_id.numeric_id());
    Microsoft::WRL::ComPtr<IDataPoint> point;
    teleclient_->RequestPoint(base::win::ScopedBstr(address.c_str()),
                              point.GetAddressOf());
    if (!point)
      return nullptr;
    return std::make_unique<VidiconMonitoredDataPoint>(std::move(point));
  }

  if (read_value_id.attribute_id == scada::AttributeId::EventNotifier) {
    assert(read_value_id.node_id == scada::id::RootFolder);
    return std::make_unique<VidiconMonitoredEvents>();
  }

  return nullptr;
}

void VidiconSession::Read(const std::vector<scada::ReadValueId>& nodes,
                          const scada::ReadCallback& callback) {
  assert(false);
}

void VidiconSession::Write(const scada::NodeId& node_id,
                           double value,
                           const scada::NodeId& user_id,
                           const scada::WriteFlags& flags,
                           const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad);
}

void VidiconSession::Acknowledge(int acknowledge_id,
                                 const scada::NodeId& user_node_id) {}

void VidiconSession::GenerateEvent(const scada::Event& event) {}

void VidiconSession::Call(const scada::NodeId& node_id,
                          const scada::NodeId& method_id,
                          const std::vector<scada::Variant>& arguments,
                          const scada::NodeId& user_id,
                          const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad);
}

void VidiconSession::CreateNode(const scada::NodeId& requested_id,
                                const scada::NodeId& parent_id,
                                scada::NodeClass node_class,
                                const scada::NodeId& type_id,
                                scada::NodeAttributes attributes,
                                const CreateNodeCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void VidiconSession::ModifyNodes(
    const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>&
        attributes,
    const scada::ModifyNodesCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void VidiconSession::DeleteNode(const scada::NodeId& node_id,
                                bool return_relations,
                                const scada::DeleteNodeCallback& callback) {
  callback(scada::StatusCode::Bad, nullptr);
}

void VidiconSession::ChangeUserPassword(
    const scada::NodeId& user_node_id,
    const scada::LocalizedText& current_password,
    const scada::LocalizedText& new_password,
    const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad);
}

void VidiconSession::AddReference(const scada::NodeId& reference_type_id,
                                  const scada::NodeId& source_id,
                                  const scada::NodeId& target_id,
                                  const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad);
}

void VidiconSession::DeleteReference(const scada::NodeId& reference_type_id,
                                     const scada::NodeId& source_id,
                                     const scada::NodeId& target_id,
                                     const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad);
}

void VidiconSession::Browse(const std::vector<scada::BrowseDescription>& nodes,
                            const scada::BrowseCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void VidiconSession::TranslateBrowsePath(
    const scada::NodeId& starting_node_id,
    const scada::RelativePath& relative_path,
    const scada::TranslateBrowsePathCallback& callback) {
  callback(scada::StatusCode::Bad, {}, 0);
}

void VidiconSession::Subscribe(scada::ViewEvents& events) {}

void VidiconSession::Unsubscribe(scada::ViewEvents& events) {}
