#include "vidicon_session.h"

#include "base/strings/stringprintf.h"
#include "base/win/scoped_bstr.h"
#include "core/monitored_item.h"
#include "vidicon/vidicon_monitored_data_point.h"
#include "vidicon/vidicon_monitored_events.h"

namespace {

base::win::ScopedComPtr<IClient> CreateTeleClient() {
  base::win::ScopedComPtr<IClient> client;
  client.CreateInstance(CLSID_Client);
  return client;
}

}  // namespace

VidiconSession::VidiconSession() : teleclient_(CreateTeleClient()) {}

VidiconSession::~VidiconSession() {}

void VidiconSession::Connect(const std::string& connection_string,
                             const std::string& username,
                             const std::string& password,
                             bool allow_remote_logoff,
                             ConnectCallback callback) {
  callback(scada::StatusCode::Good);
}

bool VidiconSession::IsConnected() const {
  return true;
}

bool VidiconSession::IsAdministrator() const {
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

void VidiconSession::HistoryRead(const scada::ReadValueId& read_value_id,
                                 base::Time from,
                                 base::Time to,
                                 const scada::Filter& filter,
                                 const scada::HistoryReadCallback& callback) {
  callback(scada::StatusCode::Bad, nullptr, nullptr);
}

std::unique_ptr<scada::MonitoredItem> VidiconSession::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id) {
  if (read_value_id.attribute_id == scada::AttributeId::Value) {
    if (read_value_id.node_id.type() != scada::NodeIdType::Numeric)
      return nullptr;
    auto address =
        base::StringPrintf(L"CF:%d", read_value_id.node_id.numeric_id());
    base::win::ScopedComPtr<IDataPoint> point;
    teleclient_->RequestPoint(base::win::ScopedBstr(address.c_str()),
                              point.Receive());
    if (!point)
      return nullptr;
    return std::make_unique<VidiconMonitoredDataPoint>(std::move(point));
  }

  if (read_value_id.attribute_id == scada::AttributeId::EventNotifier) {
    assert(read_value_id.node_id.is_null());
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
