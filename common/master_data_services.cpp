#include "master_data_services.h"

#include "base/check.h"
#include "scada/item_factory_subscription.h"
#include "scada/monitored_item.h"
#include "scada/monitoring_parameters.h"
#include "scada/standard_node_ids.h"

// MasterDataServices::MasterMonitoredItem

class MasterDataServices::MasterMonitoredItem : public scada::MonitoredItem {
 public:
  MasterMonitoredItem(MasterDataServices& owner,
                      scada::ReadValueId read_value_id,
                      scada::MonitoringParameters params)
      : owner_{&owner},
        read_value_id_{std::move(read_value_id)},
        params_{std::move(params)} {
    owner_->monitored_items_.emplace_back(this);
  }

  ~MasterMonitoredItem() {
    if (owner_) {
      auto i = std::ranges::find(owner_->monitored_items_, this);
      base::Check(i != owner_->monitored_items_.end());
      owner_->monitored_items_.erase(i);
    }
  }

  virtual void Subscribe(scada::MonitoredItemHandler handler) override {
    base::Check(!handler_);

    if (!owner_) {
      return;
    }

    handler_ = std::move(handler);

    Reconnect();
  }

  void Reconnect() {
    base::Check(owner_);

    if (!handler_.has_value()) {
      return;
    }

    if (!owner_->connected_) {
      if (const auto* data_change_handler =
              std::get_if<scada::DataChangeHandler>(&*handler_)) {
        (*data_change_handler)(
            {scada::StatusCode::Uncertain_Disconnected, base::Time::Now()});
      }
      return;
    }

    // A session without a monitored-item service is handled below via the
    // null |underlying_item_|.
    underlying_item_ =
        owner_->underlying_item_adapter_
            ? owner_->underlying_item_adapter_->CreateMonitoredItem(
                  read_value_id_, params_)
            : nullptr;

    if (!underlying_item_) {
      if (const auto* data_change_handler =
              std::get_if<scada::DataChangeHandler>(&*handler_)) {
        (*data_change_handler)({scada::StatusCode::Bad, base::Time::Now()});
      } else if (const auto* event_handler =
                     std::get_if<scada::EventHandler>(&*handler_)) {
        (*event_handler)(scada::StatusCode::Bad, {});
      }
      return;
    }

    underlying_item_->Subscribe(*handler_);
  }

  void Disconnect() {
    underlying_item_.reset();
    // TODO: Notify data change?
  }

  void DestroyOwner() { owner_ = nullptr; }

 private:
  MasterDataServices* owner_ = nullptr;
  const scada::ReadValueId read_value_id_;
  const scada::MonitoringParameters params_;
  std::shared_ptr<scada::MonitoredItem> underlying_item_;
  std::optional<scada::MonitoredItemHandler> handler_;
};

// MasterDataServices

MasterDataServices::MasterDataServices() = default;

MasterDataServices::MasterDataServices(AnyExecutor executor)
    : executor_{std::move(executor)} {}

MasterDataServices::~MasterDataServices() {
  for (auto* monitored_item : monitored_items_) {
    monitored_item->DestroyOwner();
  }
}

scada::services MasterDataServices::as_services() {
  return {.attribute_service = this,
          .monitored_item_service = this,
          .method_service = this,
          .history_service = this,
          .view_service = this,
          .node_management_service = this,
          .session_service = this,
          .monitored_item_executor = executor_};
}

void MasterDataServices::ResetCoroutineAdapters() {
  attribute_service_ = nullptr;
  view_service_ = nullptr;
  session_service_ = nullptr;
  method_service_ = nullptr;
  history_service_ = nullptr;
  node_management_service_ = nullptr;
}

void MasterDataServices::RefreshCoroutineServices() {
  ResetCoroutineAdapters();

  attribute_service_ = services_.attribute_service_.get();

  view_service_ = services_.view_service_.get();

  session_service_ = services_.session_service_.get();

  method_service_ = services_.method_service_.get();

  history_service_ = services_.history_service_.get();

  node_management_service_ = services_.node_management_service_.get();
}

void MasterDataServices::SetServices(DataServices&& services) {
  if (connected_) {
    for (auto* monitored_item : monitored_items_) {
      monitored_item->Disconnect();
    }

    connected_ = false;
    services_ = {};
    underlying_item_adapter_.reset();
    ResetCoroutineAdapters();

    session_state_changed_connection_.disconnect();

    session_state_changed_signal_(false, scada::StatusCode::Good);
  }

  services_ = std::move(services);
  RefreshCoroutineServices();
  connected_ = session_service_ != nullptr;

  if (connected_ && services_.monitored_item_service_) {
    underlying_item_adapter_ =
        std::make_unique<scada::LegacyMonitoredItemAdapter>(
            executor_, *services_.monitored_item_service_);
  }

  if (connected_) {
    if (session_service_) {
      session_state_changed_connection_ =
          session_service_->SubscribeSessionStateChanged(
              [this](bool connected, const scada::Status& status) {
                session_state_changed_signal_(connected, status);
              });
    }

    session_state_changed_signal_(true, scada::StatusCode::Good);

    for (auto* monitored_item : monitored_items_) {
      monitored_item->Reconnect();
    }
  }
}

Awaitable<void> MasterDataServices::Connect(
    scada::SessionConnectParams params) {
  (void)co_await ConnectStatus(std::move(params));
}

Awaitable<scada::Status> MasterDataServices::ConnectStatus(
    scada::SessionConnectParams params) {
  if (!session_service_) {
    co_return scada::StatusCode::Bad_Disconnected;
  }

  co_return co_await session_service_->ConnectStatus(std::move(params));
}

Awaitable<void> MasterDataServices::Disconnect() {
  if (!session_service_)
    co_return;

  co_await session_service_->Disconnect();
}

Awaitable<void> MasterDataServices::Reconnect() {
  if (!session_service_)
    co_return;

  co_await session_service_->Reconnect();
}

bool MasterDataServices::IsConnected(base::TimeDelta* ping_delay) const {
  if (!connected_)
    return false;

  return session_service_ && session_service_->IsConnected(ping_delay);
}

bool MasterDataServices::HasPrivilege(scada::Privilege privilege) const {
  if (!session_service_)
    return false;

  return session_service_->HasPrivilege(privilege);
}

scada::NodeId MasterDataServices::GetUserId() const {
  if (!session_service_)
    return {};

  return session_service_->GetUserId();
}

std::string MasterDataServices::GetHostName() const {
  if (!session_service_)
    return {};

  return session_service_->GetHostName();
}

bool MasterDataServices::IsScada() const {
  if (!session_service_)
    return false;

  return session_service_->IsScada();
}

boost::signals2::scoped_connection
MasterDataServices::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& callback) {
  return session_state_changed_signal_.connect(callback);
}

scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
MasterDataServices::CreateSubscription(
    scada::ServiceContext /*context*/,
    scada::MonitoredItemSubscriptionOptions options) {
  return scada::MakeItemFactorySubscription(
      [this](const scada::ReadValueId& read_value_id,
             const scada::MonitoringParameters& params) {
        return CreateItem(read_value_id, params);
      },
      options);
}

std::shared_ptr<scada::MonitoredItem> MasterDataServices::CreateItem(
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params) {
  return std::make_shared<MasterMonitoredItem>(*this, read_value_id, params);
}

scada::SessionDebugger* MasterDataServices::GetSessionDebugger() {
  return session_service_ ? session_service_->GetSessionDebugger() : nullptr;
}

Awaitable<scada::StatusOr<std::vector<scada::AddNodesResult>>>
MasterDataServices::AddNodes(scada::ServiceContext context,
                             std::vector<scada::AddNodesItem> inputs) {
  auto* service = node_management_service_;
  if (service)
    co_return co_await service->AddNodes(std::move(context), std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
MasterDataServices::DeleteNodes(scada::ServiceContext context,
                                std::vector<scada::DeleteNodesItem> inputs) {
  auto* service = node_management_service_;
  if (service)
    co_return co_await service->DeleteNodes(std::move(context),
                                            std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
MasterDataServices::AddReferences(
    scada::ServiceContext context,
    std::vector<scada::AddReferencesItem> inputs) {
  auto* service = node_management_service_;
  if (service)
    co_return co_await service->AddReferences(std::move(context),
                                              std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
MasterDataServices::DeleteReferences(
    scada::ServiceContext context,
    std::vector<scada::DeleteReferencesItem> inputs) {
  auto* service = node_management_service_;
  if (service)
    co_return co_await service->DeleteReferences(std::move(context),
                                                 std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
MasterDataServices::Browse(scada::ServiceContext context,
                           std::vector<scada::BrowseDescription> inputs) {
  auto* service = view_service_;
  if (service)
    co_return co_await service->Browse(std::move(context), std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
MasterDataServices::TranslateBrowsePaths(
    std::vector<scada::BrowsePath> inputs) {
  auto* service = view_service_;
  if (service)
    co_return co_await service->TranslateBrowsePaths(std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::DataValue>>>
MasterDataServices::Read(scada::ServiceContext context,
                         std::vector<scada::ReadValueId> inputs) {
  auto* service = attribute_service_;
  if (service)
    co_return co_await service->Read(std::move(context), std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
MasterDataServices::Write(scada::ServiceContext context,
                          std::vector<scada::WriteValue> inputs) {
  auto* service = attribute_service_;
  if (service)
    co_return co_await service->Write(std::move(context), std::move(inputs));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::Status> MasterDataServices::Call(
    scada::NodeId node_id,
    scada::NodeId method_id,
    std::vector<scada::Variant> arguments,
    scada::ServiceContext context) {
  auto* service = method_service_;
  if (service)
    co_return co_await service->Call(std::move(node_id), std::move(method_id),
                                     std::move(arguments), std::move(context));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::HistoryReadRawResult> MasterDataServices::HistoryReadRaw(
    scada::HistoryReadRawDetails details) {
  auto* service = history_service_;
  if (service)
    co_return co_await service->HistoryReadRaw(std::move(details));

  co_return scada::HistoryReadRawResult{
      .status = scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::HistoryReadEventsResult> MasterDataServices::HistoryReadEvents(
    scada::NodeId node_id,
    base::Time from,
    base::Time to,
    scada::EventFilter filter) {
  auto* service = history_service_;
  if (service)
    co_return co_await service->HistoryReadEvents(std::move(node_id), from, to,
                                                  std::move(filter));

  co_return scada::HistoryReadEventsResult{
      .status = scada::StatusCode::Bad_Disconnected};
}
