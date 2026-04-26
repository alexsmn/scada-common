#include "master_data_services.h"

#include "base/awaitable_promise.h"
#include "scada/monitored_item.h"
#include "scada/monitoring_parameters.h"
#include "scada/standard_node_ids.h"
#include "scada/status_exception.h"
#include "scada/status_promise.h"

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
      assert(i != owner_->monitored_items_.end());
      owner_->monitored_items_.erase(i);
    }
  }

  virtual void Subscribe(scada::MonitoredItemHandler handler) override {
    assert(!handler_);

    if (!owner_) {
      return;
    }

    handler_ = std::move(handler);

    Reconnect();
  }

  void Reconnect() {
    assert(owner_);
    assert(handler_.has_value());

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

    assert(owner_->services_.monitored_item_service_);

    underlying_item_ =
        owner_->services_.monitored_item_service_->CreateMonitoredItem(
            read_value_id_, params_);

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

class MasterDataServices::CoroutineSessionFacade final
    : public scada::CoroutineSessionService {
 public:
  explicit CoroutineSessionFacade(MasterDataServices& owner)
      : owner_{owner} {}

  Awaitable<void> Connect(scada::SessionConnectParams params) override {
    co_await owner_.ConnectCoroutine(std::move(params));
  }

  Awaitable<void> Reconnect() override {
    co_await owner_.ReconnectCoroutine();
  }

  Awaitable<void> Disconnect() override {
    co_await owner_.DisconnectCoroutine();
  }

  bool IsConnected(base::TimeDelta* ping_delay = nullptr) const override {
    return owner_.IsConnected(ping_delay);
  }

  scada::NodeId GetUserId() const override {
    return owner_.GetUserId();
  }

  bool HasPrivilege(scada::Privilege privilege) const override {
    return owner_.HasPrivilege(privilege);
  }

  std::string GetHostName() const override {
    return owner_.GetHostName();
  }

  bool IsScada() const override {
    return owner_.IsScada();
  }

  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override {
    return owner_.SubscribeSessionStateChanged(callback);
  }

  scada::SessionDebugger* GetSessionDebugger() override {
    return owner_.GetSessionDebugger();
  }

 private:
  MasterDataServices& owner_;
};

MasterDataServices::MasterDataServices()
    : coroutine_session_facade_{std::make_unique<CoroutineSessionFacade>(
          *this)} {}

MasterDataServices::MasterDataServices(AnyExecutor executor)
    : coroutine_executor_{std::move(executor)},
      coroutine_session_facade_{
          std::make_unique<CoroutineSessionFacade>(*this)} {}

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
          .session_service = this};
}

void MasterDataServices::ResetCoroutineAdapters() {
  coroutine_attribute_service_ = nullptr;
  coroutine_view_service_ = nullptr;
  coroutine_session_service_ = nullptr;
  coroutine_method_service_ = nullptr;
  coroutine_history_service_ = nullptr;
  coroutine_node_management_service_ = nullptr;

  attribute_service_adapter_.reset();
  view_service_adapter_.reset();
  session_service_adapter_.reset();
  method_service_adapter_.reset();
  history_service_adapter_.reset();
  node_management_service_adapter_.reset();
}

void MasterDataServices::RefreshCoroutineServices() {
  ResetCoroutineAdapters();

  if (services_.attribute_service_) {
    coroutine_attribute_service_ =
        dynamic_cast<scada::CoroutineAttributeService*>(
            services_.attribute_service_.get());
    if (!coroutine_attribute_service_ && coroutine_executor_) {
      attribute_service_adapter_ =
          std::make_unique<scada::CallbackToCoroutineAttributeServiceAdapter>(
              *coroutine_executor_, *services_.attribute_service_);
      coroutine_attribute_service_ = attribute_service_adapter_.get();
    }
  }

  if (services_.view_service_) {
    coroutine_view_service_ = dynamic_cast<scada::CoroutineViewService*>(
        services_.view_service_.get());
    if (!coroutine_view_service_ && coroutine_executor_) {
      view_service_adapter_ =
          std::make_unique<scada::CallbackToCoroutineViewServiceAdapter>(
              *coroutine_executor_, *services_.view_service_);
      coroutine_view_service_ = view_service_adapter_.get();
    }
  }

  if (services_.session_service_) {
    coroutine_session_service_ =
        dynamic_cast<scada::CoroutineSessionService*>(
            services_.session_service_.get());
    if (!coroutine_session_service_ && coroutine_executor_) {
      session_service_adapter_ =
          std::make_unique<scada::PromiseToCoroutineSessionServiceAdapter>(
              *coroutine_executor_, *services_.session_service_);
      coroutine_session_service_ = session_service_adapter_.get();
    }
  }

  if (services_.method_service_) {
    coroutine_method_service_ = dynamic_cast<scada::CoroutineMethodService*>(
        services_.method_service_.get());
    if (!coroutine_method_service_ && coroutine_executor_) {
      method_service_adapter_ =
          std::make_unique<scada::CallbackToCoroutineMethodServiceAdapter>(
              *coroutine_executor_, *services_.method_service_);
      coroutine_method_service_ = method_service_adapter_.get();
    }
  }

  if (services_.history_service_) {
    coroutine_history_service_ = dynamic_cast<scada::CoroutineHistoryService*>(
        services_.history_service_.get());
    if (!coroutine_history_service_ && coroutine_executor_) {
      history_service_adapter_ =
          std::make_unique<scada::CallbackToCoroutineHistoryServiceAdapter>(
              *coroutine_executor_, *services_.history_service_);
      coroutine_history_service_ = history_service_adapter_.get();
    }
  }

  if (services_.node_management_service_) {
    coroutine_node_management_service_ =
        dynamic_cast<scada::CoroutineNodeManagementService*>(
            services_.node_management_service_.get());
    if (!coroutine_node_management_service_ && coroutine_executor_) {
      node_management_service_adapter_ = std::make_unique<
          scada::CallbackToCoroutineNodeManagementServiceAdapter>(
          *coroutine_executor_, *services_.node_management_service_);
      coroutine_node_management_service_ =
          node_management_service_adapter_.get();
    }
  }
}

void MasterDataServices::SetServices(DataServices&& services) {
  if (connected_) {
    for (auto* monitored_item : monitored_items_) {
      monitored_item->Disconnect();
    }

    connected_ = false;
    services_ = {};
    ResetCoroutineAdapters();

    session_state_changed_connection_.disconnect();

    session_state_changed_signal_(false, scada::StatusCode::Good);
  }

  services_ = std::move(services);
  RefreshCoroutineServices();
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

    for (auto* monitored_item : monitored_items_) {
      monitored_item->Reconnect();
    }
  }
}

promise<void> MasterDataServices::Connect(
    const scada::SessionConnectParams& params) {
  if (coroutine_executor_ && coroutine_session_service_) {
    return ToPromise(*coroutine_executor_,
                     ConnectCoroutine(scada::SessionConnectParams{params}));
  }

  if (!services_.session_service_) {
    return MakeRejectedStatusPromise(scada::StatusCode::Bad_Disconnected);
  }

  return services_.session_service_->Connect(params);
}

promise<void> MasterDataServices::Disconnect() {
  if (coroutine_executor_ && coroutine_session_service_)
    return ToPromise(*coroutine_executor_, DisconnectCoroutine());

  if (!services_.session_service_)
    return MakeRejectedStatusPromise(scada::StatusCode::Bad_Disconnected);

  return services_.session_service_->Disconnect();
}

promise<void> MasterDataServices::Reconnect() {
  if (coroutine_executor_ && coroutine_session_service_)
    return ToPromise(*coroutine_executor_, ReconnectCoroutine());

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
  if (coroutine_executor_ && coroutine_node_management_service_) {
    CoSpawn(*coroutine_executor_,
            [this, inputs, callback]() mutable -> Awaitable<void> {
              try {
                auto [status, results] = co_await AddNodes(std::move(inputs));
                callback(std::move(status), std::move(results));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->AddNodes(inputs, callback);
}

void MasterDataServices::DeleteNodes(
    const std::vector<scada::DeleteNodesItem>& inputs,
    const scada::DeleteNodesCallback& callback) {
  if (coroutine_executor_ && coroutine_node_management_service_) {
    CoSpawn(*coroutine_executor_,
            [this, inputs, callback]() mutable -> Awaitable<void> {
              try {
                auto [status, results] =
                    co_await DeleteNodes(std::move(inputs));
                callback(std::move(status), std::move(results));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->DeleteNodes(inputs, callback);
}

void MasterDataServices::AddReferences(
    const std::vector<scada::AddReferencesItem>& inputs,
    const scada::AddReferencesCallback& callback) {
  if (coroutine_executor_ && coroutine_node_management_service_) {
    CoSpawn(*coroutine_executor_,
            [this, inputs, callback]() mutable -> Awaitable<void> {
              try {
                auto [status, results] =
                    co_await AddReferences(std::move(inputs));
                callback(std::move(status), std::move(results));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->AddReferences(inputs, callback);
}

void MasterDataServices::DeleteReferences(
    const std::vector<scada::DeleteReferencesItem>& inputs,
    const scada::DeleteReferencesCallback& callback) {
  if (coroutine_executor_ && coroutine_node_management_service_) {
    CoSpawn(*coroutine_executor_,
            [this, inputs, callback]() mutable -> Awaitable<void> {
              try {
                auto [status, results] =
                    co_await DeleteReferences(std::move(inputs));
                callback(std::move(status), std::move(results));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  if (!services_.node_management_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.node_management_service_->DeleteReferences(inputs, callback);
}

void MasterDataServices::Browse(
    const scada::ServiceContext& context,
    const std::vector<scada::BrowseDescription>& nodes,
    const scada::BrowseCallback& callback) {
  if (coroutine_executor_ && coroutine_view_service_) {
    CoSpawn(*coroutine_executor_,
            [this, context, nodes, callback]() mutable -> Awaitable<void> {
              try {
                auto [status, results] = co_await Browse(context, nodes);
                callback(std::move(status), std::move(results));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  if (!services_.view_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.view_service_->Browse(context, nodes, callback);
}

void MasterDataServices::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  if (coroutine_executor_ && coroutine_view_service_) {
    CoSpawn(*coroutine_executor_,
            [this, browse_paths, callback]() mutable -> Awaitable<void> {
              try {
                auto [status, results] =
                    co_await TranslateBrowsePaths(browse_paths);
                callback(std::move(status), std::move(results));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

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
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
    const scada::WriteCallback& callback) {
  if (coroutine_executor_ && coroutine_attribute_service_) {
    CoSpawn(*coroutine_executor_,
            [this, context, inputs, callback]() mutable -> Awaitable<void> {
              try {
                auto [status, results] = co_await Write(context, inputs);
                callback(std::move(status), std::move(results));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  if (!services_.attribute_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.attribute_service_->Write(context, inputs, callback);
}

void MasterDataServices::Call(const scada::NodeId& node_id,
                              const scada::NodeId& method_id,
                              const std::vector<scada::Variant>& arguments,
                              const scada::NodeId& user_id,
                              const scada::StatusCallback& callback) {
  if (coroutine_executor_ && coroutine_method_service_) {
    CoSpawn(*coroutine_executor_,
            [this, node_id, method_id, arguments, user_id,
             callback]() mutable -> Awaitable<void> {
              try {
                callback(co_await Call(node_id, method_id, arguments, user_id));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()));
              }
            });
    return;
  }

  if (!services_.method_service_)
    return callback(scada::StatusCode::Bad_Disconnected);

  services_.method_service_->Call(node_id, method_id, arguments, user_id,
                                  callback);
}

void MasterDataServices::HistoryReadRaw(
    const scada::HistoryReadRawDetails& details,
    const scada::HistoryReadRawCallback& callback) {
  if (coroutine_executor_ && coroutine_history_service_) {
    CoSpawn(*coroutine_executor_,
            [this, details, callback]() mutable -> Awaitable<void> {
              try {
                callback(co_await HistoryReadRaw(details));
              } catch (...) {
                callback(scada::HistoryReadRawResult{
                    .status =
                        scada::GetExceptionStatus(std::current_exception())});
              }
            });
    return;
  }

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
  if (coroutine_executor_ && coroutine_history_service_) {
    CoSpawn(*coroutine_executor_,
            [this, node_id, from, to, filter,
             callback]() mutable -> Awaitable<void> {
              try {
                callback(co_await HistoryReadEvents(node_id, from, to, filter));
              } catch (...) {
                callback(scada::HistoryReadEventsResult{
                    .status =
                        scada::GetExceptionStatus(std::current_exception())});
              }
            });
    return;
  }

  if (!services_.history_service_)
    return callback({scada::StatusCode::Bad_Disconnected});

  services_.history_service_->HistoryReadEvents(node_id, from, to, filter,
                                                callback);
}

void MasterDataServices::Read(
    const scada::ServiceContext& context,
    const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
    const scada::ReadCallback& callback) {
  if (coroutine_executor_ && coroutine_attribute_service_) {
    CoSpawn(*coroutine_executor_,
            [this, context, inputs, callback]() mutable -> Awaitable<void> {
              try {
                auto [status, results] = co_await Read(context, inputs);
                callback(std::move(status), std::move(results));
              } catch (...) {
                callback(scada::GetExceptionStatus(std::current_exception()),
                         {});
              }
            });
    return;
  }

  if (!services_.attribute_service_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  services_.attribute_service_->Read(context, inputs, callback);
}

scada::SessionDebugger* MasterDataServices::GetSessionDebugger() {
  return services_.session_service_
             ? services_.session_service_->GetSessionDebugger()
             : nullptr;
}

scada::CoroutineSessionService&
MasterDataServices::coroutine_session_service() {
  return *coroutine_session_facade_;
}

Awaitable<void> MasterDataServices::ConnectCoroutine(
    scada::SessionConnectParams params) {
  if (!coroutine_session_service_)
    throw scada::status_exception{scada::StatusCode::Bad_Disconnected};

  co_await coroutine_session_service_->Connect(std::move(params));
}

Awaitable<void> MasterDataServices::DisconnectCoroutine() {
  if (!coroutine_session_service_)
    throw scada::status_exception{scada::StatusCode::Bad_Disconnected};

  co_await coroutine_session_service_->Disconnect();
}

Awaitable<void> MasterDataServices::ReconnectCoroutine() {
  if (!coroutine_session_service_)
    throw scada::status_exception{scada::StatusCode::Bad_Disconnected};

  co_await coroutine_session_service_->Reconnect();
}

Awaitable<std::tuple<scada::Status, std::vector<scada::AddNodesResult>>>
MasterDataServices::AddNodes(std::vector<scada::AddNodesItem> inputs) {
  auto* service = coroutine_node_management_service_;
  if (service)
    co_return co_await service->AddNodes(std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::AddNodesResult>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::StatusCode>>>
MasterDataServices::DeleteNodes(std::vector<scada::DeleteNodesItem> inputs) {
  auto* service = coroutine_node_management_service_;
  if (service)
    co_return co_await service->DeleteNodes(std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::StatusCode>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::StatusCode>>>
MasterDataServices::AddReferences(
    std::vector<scada::AddReferencesItem> inputs) {
  auto* service = coroutine_node_management_service_;
  if (service)
    co_return co_await service->AddReferences(std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::StatusCode>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::StatusCode>>>
MasterDataServices::DeleteReferences(
    std::vector<scada::DeleteReferencesItem> inputs) {
  auto* service = coroutine_node_management_service_;
  if (service)
    co_return co_await service->DeleteReferences(std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::StatusCode>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::BrowseResult>>>
MasterDataServices::Browse(scada::ServiceContext context,
                           std::vector<scada::BrowseDescription> inputs) {
  auto* service = coroutine_view_service_;
  if (service)
    co_return co_await service->Browse(std::move(context), std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::BrowseResult>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::BrowsePathResult>>>
MasterDataServices::TranslateBrowsePaths(
    std::vector<scada::BrowsePath> inputs) {
  auto* service = coroutine_view_service_;
  if (service)
    co_return co_await service->TranslateBrowsePaths(std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::BrowsePathResult>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::DataValue>>>
MasterDataServices::Read(
    scada::ServiceContext context,
    std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) {
  auto* service = coroutine_attribute_service_;
  if (service)
    co_return co_await service->Read(std::move(context), std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::DataValue>{}};
}

Awaitable<std::tuple<scada::Status, std::vector<scada::StatusCode>>>
MasterDataServices::Write(
    scada::ServiceContext context,
    std::shared_ptr<const std::vector<scada::WriteValue>> inputs) {
  auto* service = coroutine_attribute_service_;
  if (service)
    co_return co_await service->Write(std::move(context), std::move(inputs));

  co_return std::tuple{scada::Status{scada::StatusCode::Bad_Disconnected},
                       std::vector<scada::StatusCode>{}};
}

Awaitable<scada::Status> MasterDataServices::Call(
    scada::NodeId node_id,
    scada::NodeId method_id,
    std::vector<scada::Variant> arguments,
    scada::NodeId user_id) {
  auto* service = coroutine_method_service_;
  if (service)
    co_return co_await service->Call(std::move(node_id), std::move(method_id),
                                     std::move(arguments), std::move(user_id));

  co_return scada::Status{scada::StatusCode::Bad_Disconnected};
}

Awaitable<scada::HistoryReadRawResult> MasterDataServices::HistoryReadRaw(
    scada::HistoryReadRawDetails details) {
  auto* service = coroutine_history_service_;
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
  auto* service = coroutine_history_service_;
  if (service)
    co_return co_await service->HistoryReadEvents(
        std::move(node_id), from, to, std::move(filter));

  co_return scada::HistoryReadEventsResult{
      .status = scada::StatusCode::Bad_Disconnected};
}
