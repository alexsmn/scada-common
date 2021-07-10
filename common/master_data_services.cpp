#include "master_data_services.h"

#include "core/monitored_item.h"
#include "core/standard_node_ids.h"

// MasterDataServices::MasterMonitoredItem

class MasterDataServices::MasterMonitoredItem : public scada::MonitoredItem {
 public:
  MasterMonitoredItem(MasterDataServices& owner,
                      scada::ReadValueId read_value_id,
                      scada::MonitoringParameters params)
      : owner_{owner},
        read_value_id_{std::move(read_value_id)},
        params_{std::move(params)} {
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
            read_value_id_, params_);
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
          [this](const scada::Status& status, const std::any& event) {
            ForwardEvent(status, event);
          });
    }

    underlying_item_->Subscribe();
  }

 private:
  MasterDataServices& owner_;
  const scada::ReadValueId read_value_id_;
  const scada::MonitoringParameters params_;
  std::shared_ptr<scada::MonitoredItem> underlying_item_;
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

    if (services_.session_service_)
      services_.session_service_->RemoveObserver(*this);
  }

  services_ = std::move(services);
  connected_ = services_.session_service_ != nullptr;

  if (connected_) {
    if (services_.session_service_)
      services_.session_service_->AddObserver(*this);

    OnSessionCreated();

    for (auto* monitored_item : monitored_items_)
      monitored_item->Reconnect();
  }
}

void MasterDataServices::Connect(const std::string& host,
                                 const scada::LocalizedText& user_name,
                                 const scada::LocalizedText& password,
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

void MasterDataServices::Disconnect(const scada::StatusCallback& callback) {
  if (!services_.session_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.session_service_->Disconnect(callback);
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

void MasterDataServices::AddNodes(
    const std::vector<scada::AddNodesItem>& inputs,
    const scada::AddNodesCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->AddNodes(inputs, callback);
}

void MasterDataServices::DeleteNodes(
    const std::vector<scada::DeleteNodesItem>& inputs,
    const scada::DeleteNodesCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->DeleteNodes(inputs, callback);
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

void MasterDataServices::AddReferences(
    const std::vector<scada::AddReferencesItem>& inputs,
    const scada::AddReferencesCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->AddReferences(inputs, callback);
}

void MasterDataServices::DeleteReferences(
    const std::vector<scada::DeleteReferencesItem>& inputs,
    const scada::DeleteReferencesCallback& callback) {
  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->DeleteReferences(inputs, callback);
}

void MasterDataServices::Browse(
    const std::vector<scada::BrowseDescription>& nodes,
    const scada::BrowseCallback& callback) {
  if (!services_.view_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.view_service_->Browse(nodes, callback);
}

void MasterDataServices::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  if (!services_.view_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.view_service_->TranslateBrowsePaths(browse_paths, callback);
}

void MasterDataServices::Acknowledge(base::span<const int> acknowledge_ids,
                                     const scada::NodeId& user_id) {
  if (!services_.event_service_)
    return;

  services_.event_service_->Acknowledge(acknowledge_ids, user_id);
}

std::shared_ptr<scada::MonitoredItem> MasterDataServices::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params) {
  return std::make_shared<MasterMonitoredItem>(*this, read_value_id, params);
}

void MasterDataServices::Write(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  if (!services_.attribute_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.attribute_service_->Write(context, inputs, callback);
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
    const scada::HistoryReadRawDetails& details,
    const scada::HistoryReadRawCallback& callback) {
  if (!services_.history_service_)
    return callback({scada::StatusCode::Bad_Disconnected});

  services_.history_service_->HistoryReadRaw(details, callback);
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

void MasterDataServices::Read(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  if (!services_.attribute_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.attribute_service_->Read(context, inputs, callback);
}

void MasterDataServices::OnSessionCreated() {
  for (auto& obs : session_state_observers_)
    obs.OnSessionCreated();
}

void MasterDataServices::OnSessionDeleted(const scada::Status& status) {
  for (auto& obs : session_state_observers_)
    obs.OnSessionDeleted(status);
}
