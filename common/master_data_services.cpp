#include "master_data_services.h"

#include "core/monitored_item.h"

MasterDataServices::MasterDataServices() {
}

MasterDataServices::~MasterDataServices() {
  SetServices({});
}

void MasterDataServices::SetServices(const DataServices& services) {
  if (services_.view_service_)
    services_.view_service_->Unsubscribe(*this);
  if (services_.session_service_)
    services_.session_service_->RemoveObserver(*this);

  services_ = services;

  if (services_.view_service_)
    services_.view_service_->Subscribe(*this);
  if (services_.session_service_)
    services_.session_service_->AddObserver(*this);
}

void MasterDataServices::Connect(const std::string& host,
                           const std::string& username, const std::string& password,
                           bool allow_remote_logoff, ConnectCallback callback) {
  if (!services_.session_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.session_service_->Connect(host, username, password, allow_remote_logoff, std::move(callback));
}

bool MasterDataServices::IsConnected() const {
  if (!services_.session_service_)
    return false;

  return services_.session_service_->IsConnected();
}

bool MasterDataServices::IsAdministrator() const {
  if (!services_.session_service_)
    return false;

  return services_.session_service_->IsAdministrator();
}

scada::NodeId MasterDataServices::GetUserId() const {
  if (!services_.session_service_)
    return {};

  return services_.session_service_->GetUserId();
}

std::string MasterDataServices::GetHostName() const {
  if (!services_.session_service_)
    return {};

  return services_.session_service_->GetHostName();
}

bool MasterDataServices::IsScada() const {
  if (!services_.session_service_)
    return false;

  return services_.session_service_->IsScada();
}

void MasterDataServices::AddObserver(scada::SessionStateObserver& observer) {
  session_state_observers_.AddObserver(&observer);
}

void MasterDataServices::RemoveObserver(scada::SessionStateObserver& observer) {
  session_state_observers_.RemoveObserver(&observer);
}

void MasterDataServices::CreateNode(const scada::NodeId& requested_id, const scada::NodeId& parent_id,
                                          scada::NodeClass node_class, const scada::NodeId& type_id,
                                          scada::NodeAttributes attributes, const CreateNodeCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->CreateNode(requested_id, parent_id, node_class, type_id, std::move(attributes), callback);
}

void MasterDataServices::ModifyNodes(const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>& attributes,
                                          const MultiStatusCallback& callback) {
  if (!services_.node_management_service_)
    return callback(std::vector<scada::Status>(attributes.size(), scada::StatusCode::Bad_Disconnected));

  services_.node_management_service_->ModifyNodes(attributes, callback);
}

void MasterDataServices::DeleteNode(const scada::NodeId& node_id,
                                          bool return_references,
                                          const DeleteNodeCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, nullptr);

  services_.node_management_service_->DeleteNode(node_id, return_references, callback);
}

void MasterDataServices::ChangeUserPassword(const scada::NodeId& user_id,
                                                  const std::string& current_password,
                                                  const std::string& new_password,
                                                  const StatusCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.node_management_service_->ChangeUserPassword(user_id, current_password, new_password, callback);
}

void MasterDataServices::AddReference(const scada::NodeId& reference_type_id,
                                            const scada::NodeId& source_id,
                                            const scada::NodeId& target_id,
                                            const StatusCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.node_management_service_->AddReference(reference_type_id, source_id, target_id, callback);
}

void MasterDataServices::DeleteReference(const scada::NodeId& reference_type_id,
                                        const scada::NodeId& source_id,
                                        const scada::NodeId& target_id,
                                        const StatusCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.node_management_service_->DeleteReference(reference_type_id, source_id, target_id, callback);
}

void MasterDataServices::Browse(const std::vector<scada::BrowseDescription>& nodes, const scada::BrowseCallback& callback) {
  if (!services_.view_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.view_service_->Browse(nodes, callback);
}

void MasterDataServices::TranslateBrowsePath(const scada::NodeId& starting_node_id,
    const scada::RelativePath& relative_path, const scada::TranslateBrowsePathCallback& callback) {
  if (!services_.view_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {}, 0);

  services_.view_service_->TranslateBrowsePath(starting_node_id, relative_path, callback);
}

void MasterDataServices::Subscribe(scada::ViewEvents& events) {
  view_events_.AddObserver(&events);
}

void MasterDataServices::Unsubscribe(scada::ViewEvents& events) {
  view_events_.RemoveObserver(&events);
}

void MasterDataServices::Acknowledge(int acknowledge_id, const scada::NodeId& user_node_id) {
  if (!services_.event_service_)
    return;

  services_.event_service_->Acknowledge(acknowledge_id, user_node_id);
}

void MasterDataServices::GenerateEvent(const scada::Event& event) {
  if (!services_.event_service_)
    return;

  services_.event_service_->GenerateEvent(event);
}

std::unique_ptr<scada::MonitoredItem> MasterDataServices::CreateMonitoredItem(const scada::ReadValueId& read_value_id) {
  if (!services_.monitored_item_service_)
    return nullptr;

  return services_.monitored_item_service_->CreateMonitoredItem(read_value_id);
}

void MasterDataServices::Write(const scada::NodeId& node_id, double value, const scada::NodeId& user_id, const scada::WriteFlags& flags, const scada::StatusCallback& callback) {
  if (!services_.attribute_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.attribute_service_->Write(node_id, value, user_id, flags, callback);
}

void MasterDataServices::Call(const scada::NodeId& node_id, const scada::NodeId& method_id, const std::vector<scada::Variant>& arguments, const scada::StatusCallback& callback) {
  if (!services_.method_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.method_service_->Call(node_id, method_id, arguments, callback);
}

void MasterDataServices::QueryItemInfos(const scada::ItemInfosCallback& callback) {
  if (!services_.history_service_)
    return;

  services_.history_service_->QueryItemInfos(callback);
}

void MasterDataServices::WriteItemInfo(const scada::ItemInfo& info) {
  if (!services_.history_service_)
    return;

  services_.history_service_->WriteItemInfo(info);
}

void MasterDataServices::HistoryRead(const scada::ReadValueId& read_value_id, base::Time from, base::Time to,
                                    const scada::Filter& filter, const scada::HistoryReadCallback& callback) {
  if (!services_.history_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {}, {});

  services_.history_service_->HistoryRead(read_value_id, from, to, filter, callback);
}

void MasterDataServices::WriteEvent(const scada::Event& event) {
  if (!services_.history_service_)
    return;

  services_.history_service_->WriteEvent(event);
}

void MasterDataServices::AcknowledgeEvent(unsigned ack_id, base::Time time,
                              const scada::NodeId& user_node_id) {
  if (!services_.history_service_)
    return;

  services_.history_service_->AcknowledgeEvent(ack_id, time, user_node_id);
}

void MasterDataServices::Read(const std::vector<scada::ReadValueId>& nodes, const scada::ReadCallback& callback) {
  if (!services_.attribute_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.attribute_service_->Read(nodes, callback);
}

void MasterDataServices::OnSessionCreated() {
  for (auto& obs : session_state_observers_)
    obs.OnSessionCreated();
}

void MasterDataServices::OnSessionDeleted(const scada::Status& status) {
  for (auto& obs : session_state_observers_)
    obs.OnSessionDeleted(status);
}

void MasterDataServices::OnNodeAdded(const scada::NodeId& node_id) {
  for (auto& obs : view_events_)
    obs.OnNodeAdded(node_id);
}

void MasterDataServices::OnNodeDeleted(const scada::NodeId& node_id) {
  for (auto& obs : view_events_)
    obs.OnNodeDeleted(node_id);
}

void MasterDataServices::OnReferenceAdded(const scada::NodeId& node_id) {
  for (auto& obs : view_events_)
    obs.OnReferenceAdded(node_id);
}

void MasterDataServices::OnReferenceDeleted(const scada::NodeId& node_id) {
  for (auto& obs : view_events_)
    obs.OnReferenceDeleted(node_id);
}

void MasterDataServices::OnNodeSemanticsChanged(const scada::NodeId& node_id) {
  for (auto& obs : view_events_)
    obs.OnNodeSemanticsChanged(node_id);
}
