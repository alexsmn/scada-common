#pragma once

#include "base/any_executor.h"
#include "scada/attribute_service.h"
#include "scada/coroutine_services.h"
#include "scada/data_services.h"
#include "scada/history_service.h"
#include "scada/legacy_monitored_item_adapter.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "scada/node_management_service.h"
#include "scada/session_service.h"
#include "scada/view_service.h"

#include <boost/signals2/signal.hpp>

#include <memory>

class MasterDataServices final : public scada::AttributeService,
                                 public scada::ViewService,
                                 public scada::SessionService,
                                 public scada::MonitoredItemService,
                                 public scada::MethodService,
                                 public scada::HistoryService,
                                 public scada::NodeManagementService {
 public:
  MasterDataServices();
  explicit MasterDataServices(AnyExecutor executor);
  ~MasterDataServices();

  scada::services as_services();

  void SetServices(DataServices&& sevices);

  // scada::SessionService
  virtual Awaitable<void> Connect(scada::SessionConnectParams params) override;
  virtual Awaitable<scada::Status> ConnectStatus(
      scada::SessionConnectParams params) override;
  virtual Awaitable<void> Disconnect() override;
  virtual Awaitable<void> Reconnect() override;
  virtual bool IsConnected(
      base::TimeDelta* ping_delay = nullptr) const override;
  virtual bool HasPrivilege(scada::Privilege privilege) const override;
  virtual bool IsScada() const override;
  virtual scada::NodeId GetUserId() const override;
  virtual std::string GetHostName() const override;
  virtual boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override;
  virtual scada::SessionDebugger* GetSessionDebugger() override;

  // scada::MonitoredItemService
  scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
  CreateSubscription(scada::ServiceContext context,
                     scada::MonitoredItemSubscriptionOptions options) override;

  // scada::NodeManagementService
  [[nodiscard]] virtual Awaitable<
      scada::StatusOr<std::vector<scada::AddNodesResult>>>
  AddNodes(std::vector<scada::AddNodesItem> inputs) override;
  [[nodiscard]] virtual Awaitable<
      scada::StatusOr<std::vector<scada::StatusCode>>>
  DeleteNodes(std::vector<scada::DeleteNodesItem> inputs) override;
  [[nodiscard]] virtual Awaitable<
      scada::StatusOr<std::vector<scada::StatusCode>>>
  AddReferences(std::vector<scada::AddReferencesItem> inputs) override;
  [[nodiscard]] virtual Awaitable<
      scada::StatusOr<std::vector<scada::StatusCode>>>
  DeleteReferences(std::vector<scada::DeleteReferencesItem> inputs) override;

  // scada::ViewService
  [[nodiscard]] virtual Awaitable<
      scada::StatusOr<std::vector<scada::BrowseResult>>>
  Browse(scada::ServiceContext context,
         std::vector<scada::BrowseDescription> inputs) override;
  [[nodiscard]] virtual Awaitable<
      scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override;

  // scada::AttributeService
  [[nodiscard]] virtual Awaitable<
      scada::StatusOr<std::vector<scada::DataValue>>>
  Read(scada::ServiceContext context,
       std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) override;
  [[nodiscard]] virtual Awaitable<
      scada::StatusOr<std::vector<scada::StatusCode>>>
  Write(scada::ServiceContext context,
        std::shared_ptr<const std::vector<scada::WriteValue>> inputs) override;

  // scada::MethodService
  [[nodiscard]] virtual Awaitable<scada::Status> Call(
      scada::NodeId node_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments,
      scada::NodeId user_id) override;

  // scada::HistoryService
  [[nodiscard]] virtual Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override;
  [[nodiscard]] virtual Awaitable<scada::HistoryReadEventsResult>
  HistoryReadEvents(scada::NodeId node_id,
                    base::Time from,
                    base::Time to,
                    scada::EventFilter filter) override;

 private:
  class MasterMonitoredItem;

  std::shared_ptr<scada::MonitoredItem> CreateItem(
      const scada::ReadValueId& read_value_id,
      const scada::MonitoringParameters& params);

  void ResetCoroutineAdapters();
  void RefreshCoroutineServices();

  const AnyExecutor executor_;

  // Bridges the underlying service's subscription API to the single-item API
  // each `MasterMonitoredItem` consumes. Recreated whenever the underlying
  // services change.
  std::unique_ptr<scada::LegacyMonitoredItemAdapter> underlying_item_adapter_;

  std::vector<MasterMonitoredItem*> monitored_items_;

  boost::signals2::scoped_connection session_state_changed_connection_;

  boost::signals2::signal<void(bool connected, const scada::Status& status)>
      session_state_changed_signal_;

  DataServices services_;
  bool connected_ = false;

  scada::AttributeService* attribute_service_ = nullptr;
  scada::ViewService* view_service_ = nullptr;
  scada::SessionService* session_service_ = nullptr;
  scada::MethodService* method_service_ = nullptr;
  scada::HistoryService* history_service_ = nullptr;
  scada::NodeManagementService* node_management_service_ = nullptr;
};
