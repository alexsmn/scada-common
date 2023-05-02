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

  virtual void Subscribe(scada::MonitoredItemHandler handler) override {
    assert(!handler_);

    handler_ = std::move(handler);

    Reconnect();
  }

  void Reconnect() {
    assert(handler_.has_value());
    if (!handler_.has_value())
      return;

    if (!owner_.connected_) {
      if (auto* data_change_handler =
              std::get_if<scada::DataChangeHandler>(&*handler_)) {
        (*data_change_handler)(
            {scada::StatusCode::Uncertain_Disconnected, base::Time::Now()});
      }
      return;
    }

    assert(owner_.services_.monitored_item_service_);

    underlying_item_ =
        owner_.services_.monitored_item_service_->CreateMonitoredItem(
            read_value_id_, params_);
    if (!underlying_item_) {
      if (auto* data_change_handler =
              std::get_if<scada::DataChangeHandler>(&*handler_)) {
        (*data_change_handler)({scada::StatusCode::Bad, base::Time::Now()});
      } else if (auto* event_handler =
                     std::get_if<scada::EventHandler>(&*handler_)) {
        (*event_handler)(scada::StatusCode::Bad, {});
      }
      return;
    }

    underlying_item_->Subscribe(*handler_);
  }

 private:
  MasterDataServices& owner_;
  const scada::ReadValueId read_value_id_;
  const scada::MonitoringParameters params_;
  std::shared_ptr<scada::MonitoredItem> underlying_item_;
  std::optional<scada::MonitoredItemHandler> handler_;
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

    session_state_changed_connection_.disconnect();

    session_state_changed_signal_(false, scada::StatusCode::Good);
  }

  services_ = std::move(services);
  connected_ = services_.session_service_ != nullptr;

  if (connected_) {
    if (services_.session_service_) {
      session_state_changed_connection_ =
          services_.session_service_->SubscribeSessionStateChanged(
              [this](bool connected, const scada::Status& status) {
                session_state_changed_signal_(connected, status);
              });
    }

    session_state_changed_signal_(true, scada::StatusCode::Good);

    for (auto* monitored_item : monitored_items_)
      monitored_item->Reconnect();
  }
}

promise<> MasterDataServices::Connect(const std::string& host,
                                      const scada::LocalizedText& user_name,
                                      const scada::LocalizedText& password,
                                      bool allow_remote_logoff) {
  if (!services_.session_service_)
    return MakeRejectedStatusPromise(scada::StatusCode::Bad_Disconnected);

  return services_.session_service_->Connect(host, user_name, password,
                                             allow_remote_logoff);
}

promise<> MasterDataServices::Disconnect() {
  if (!services_.session_service_)
    return MakeRejectedStatusPromise(scada::StatusCode::Bad_Disconnected);

  return services_.session_service_->Disconnect();
}

promise<> MasterDataServices::Reconnect() {
  if (!services_.session_service_)
    return MakeRejectedStatusPromise(scada::StatusCode::Bad_Disconnected);

  return services_.session_service_->Reconnect();
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

boost::signals2::scoped_connection
MasterDataServices::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& callback) {
  return session_state_changed_signal_.connect(callback);
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
