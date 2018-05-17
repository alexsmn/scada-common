#include "master_data_services.h"

#include "core/monitored_item.h"
#include "core/standard_node_ids.h"

// MasterDataServices::MasterMonitoredItem

class MasterDataServices::MasterMonitoredItem : public scada::MonitoredItem {
 public:
  MasterMonitoredItem(MasterDataServices& owner,
                      scada::ReadValueId read_value_id)
      : owner_{owner}, read_value_id_{std::move(read_value_id)} {
    owner_.monitored_items_.emplace_back(this);
  }

  ~MasterMonitoredItem() {
    auto i = std::find(owner_.monitored_items_.begin(),
                       owner_.monitored_items_.end(), this);
    assert(i != owner_.monitored_items_.end());
    owner_.monitored_items_.erase(i);
  }

  virtual void Subscribe() override { Reconnect(); }

  void Reconnect() {
    if (!owner_.connected_) {
      ForwardData(
          {scada::StatusCode::Uncertain_Disconnected, base::Time::Now()});
      return;
    }

    assert(owner_.services_.monitored_item_service_);

    underlying_item_ =
        owner_.services_.monitored_item_service_->CreateMonitoredItem(
            read_value_id_);
    if (!underlying_item_) {
      ForwardData({scada::StatusCode::Bad, base::Time::Now()});
      return;
    }

    if (read_value_id_.attribute_id != scada::AttributeId::EventNotifier) {
      underlying_item_->set_data_change_handler(
          [this](const scada::DataValue& data_value) {
            ForwardData(data_value);
          });

    } else {
      underlying_item_->set_event_handler(
          [this](const scada::Status& status, const scada::Event& event) {
            ForwardEvent(status, event);
          });
    }

    underlying_item_->Subscribe();
  }

 private:
  MasterDataServices& owner_;
  const scada::ReadValueId read_value_id_;
  std::unique_ptr<scada::MonitoredItem> underlying_item_;
};

// MasterDataServices

MasterDataServices::MasterDataServices() {}

MasterDataServices::~MasterDataServices() {
  SetServices({});
}

void MasterDataServices::SetServices(DataServices&& services) {
  if (connected_) {
    connected_ = false;
    services_ = {};

    OnSessionDeleted(scada::StatusCode::Good);

    if (services_.view_service_)
      services_.view_service_->Unsubscribe(*this);
    if (services_.session_service_)
      services_.session_service_->RemoveObserver(*this);
  }

  services_ = std::move(services);
  connected_ = services_.session_service_ != nullptr;

  if (connected_) {
    if (services_.view_service_)
      services_.view_service_->Subscribe(*this);
    if (services_.session_service_)
      services_.session_service_->AddObserver(*this);

    OnSessionCreated();

    for (auto* monitored_item : monitored_items_)
      monitored_item->Reconnect();
  }
}

void MasterDataServices::Connect(const std::string& host,
                                 const scada::LocalizedText& user_name,
                                 const std::string& password,
                                 bool allow_remote_logoff,
                                 const scada::StatusCallback& callback) {
  if (!services_.session_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.session_service_->Connect(host, user_name, password,
                                      allow_remote_logoff, std::move(callback));
}

void MasterDataServices::Reconnect() {
  if (services_.session_service_)
    services_.session_service_->Reconnect();
}

bool MasterDataServices::IsConnected(base::TimeDelta* ping_delay) const {
  if (!connected_)
    return false;

  return services_.session_service_->IsConnected(ping_delay);
}

bool MasterDataServices::HasPrivilege(scada::Privilege privilege) const {
  if (!services_.session_service_)
    return false;

  return services_.session_service_->HasPrivilege(privilege);
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

void MasterDataServices::CreateNode(const scada::NodeId& requested_id,
                                    const scada::NodeId& parent_id,
                                    scada::NodeClass node_class,
                                    const scada::NodeId& type_id,
                                    scada::NodeAttributes attributes,
                                    const CreateNodeCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->CreateNode(
      requested_id, parent_id, node_class, type_id, std::move(attributes),
      callback);
}

void MasterDataServices::ModifyNodes(
    const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>&
        attributes,
    const scada::ModifyNodesCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->ModifyNodes(attributes, callback);
}

void MasterDataServices::DeleteNode(const scada::NodeId& node_id,
                                    bool return_references,
                                    const scada::DeleteNodeCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, nullptr);

  services_.node_management_service_->DeleteNode(node_id, return_references,
                                                 callback);
}

void MasterDataServices::ChangeUserPassword(
    const scada::NodeId& user_id,
    const scada::LocalizedText& current_password,
    const scada::LocalizedText& new_password,
    const scada::StatusCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.node_management_service_->ChangeUserPassword(
      user_id, current_password, new_password, callback);
}

void MasterDataServices::AddReference(const scada::NodeId& reference_type_id,
                                      const scada::NodeId& source_id,
                                      const scada::NodeId& target_id,
                                      const scada::StatusCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.node_management_service_->AddReference(reference_type_id, source_id,
                                                   target_id, callback);
}

void MasterDataServices::DeleteReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id,
    const scada::StatusCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.node_management_service_->DeleteReference(
      reference_type_id, source_id, target_id, callback);
}

void MasterDataServices::Browse(
    const std::vector<scada::BrowseDescription>& nodes,
    const scada::BrowseCallback& callback) {
  if (!services_.view_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.view_service_->Browse(nodes, callback);
}

void MasterDataServices::TranslateBrowsePath(
    const scada::NodeId& starting_node_id,
    const scada::RelativePath& relative_path,
    const scada::TranslateBrowsePathCallback& callback) {
  if (!services_.view_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {}, 0);

  services_.view_service_->TranslateBrowsePath(starting_node_id, relative_path,
                                               callback);
}

void MasterDataServices::Subscribe(scada::ViewEvents& events) {
  view_events_.AddObserver(&events);
}

void MasterDataServices::Unsubscribe(scada::ViewEvents& events) {
  view_events_.RemoveObserver(&events);
}

void MasterDataServices::Acknowledge(int acknowledge_id,
                                     const scada::NodeId& user_node_id) {
  if (!services_.event_service_)
    return;

  services_.event_service_->Acknowledge(acknowledge_id, user_node_id);
}

void MasterDataServices::GenerateEvent(const scada::Event& event) {
  if (!services_.event_service_)
    return;

  services_.event_service_->GenerateEvent(event);
}

std::unique_ptr<scada::MonitoredItem> MasterDataServices::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id) {
  return std::make_unique<MasterMonitoredItem>(*this, read_value_id);
}

void MasterDataServices::Write(const scada::NodeId& node_id,
                               double value,
                               const scada::NodeId& user_id,
                               const scada::WriteFlags& flags,
                               const scada::StatusCallback& callback) {
  if (!services_.attribute_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.attribute_service_->Write(node_id, value, user_id, flags, callback);
}

void MasterDataServices::Call(const scada::NodeId& node_id,
                              const scada::NodeId& method_id,
                              const std::vector<scada::Variant>& arguments,
                              const scada::NodeId& user_id,
                              const scada::StatusCallback& callback) {
  if (!services_.method_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.method_service_->Call(node_id, method_id, arguments, user_id,
                                  callback);
}

void MasterDataServices::HistoryReadRaw(
    const scada::NodeId& node_id,
    base::Time from,
    base::Time to,
    const scada::HistoryReadRawCallback& callback) {
  if (!services_.history_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.history_service_->HistoryReadRaw(node_id, from, to, callback);
}

void MasterDataServices::HistoryReadEvents(
    const scada::NodeId& node_id,
    base::Time from,
    base::Time to,
    const scada::EventFilter& filter,
    const scada::HistoryReadEventsCallback& callback) {
  if (!services_.history_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.history_service_->HistoryReadEvents(node_id, from, to, filter,
                                                callback);
}

void MasterDataServices::Read(const std::vector<scada::ReadValueId>& nodes,
                              const scada::ReadCallback& callback) {
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

void MasterDataServices::OnModelChanged(const scada::ModelChangeEvent& event) {
  for (auto& obs : view_events_)
    obs.OnModelChanged(event);
}

void MasterDataServices::OnNodeSemanticsChanged(const scada::NodeId& node_id) {
  for (auto& obs : view_events_)
    obs.OnNodeSemanticsChanged(node_id);
}
