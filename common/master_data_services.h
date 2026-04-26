#pragma once

#include "base/any_executor.h"
#include "scada/attribute_service.h"
#include "scada/coroutine_services.h"
#include "scada/data_services.h"
#include "scada/history_service.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "scada/node_management_service.h"
#include "scada/session_service.h"
#include "scada/view_service.h"

#include <boost/signals2/signal.hpp>

#include <memory>
#include <optional>

class MasterDataServices final : public scada::AttributeService,
                                 public scada::ViewService,
                                 public scada::SessionService,
                                 public scada::MonitoredItemService,
                                 public scada::MethodService,
                                 public scada::HistoryService,
                                 public scada::NodeManagementService,
                                 public scada::CoroutineAttributeService,
                                 public scada::CoroutineViewService,
                                 public scada::CoroutineMethodService,
                                 public scada::CoroutineHistoryService,
                                 public scada::CoroutineNodeManagementService {
 public:
  MasterDataServices();
  explicit MasterDataServices(AnyExecutor executor);
  ~MasterDataServices();

  scada::services as_services();

  void SetServices(DataServices&& sevices);

  // scada::SessionService
  virtual promise<void> Connect(
      const scada::SessionConnectParams& params) override;
  virtual promise<void> Disconnect() override;
  virtual promise<void> Reconnect() override;
  virtual bool IsConnected(
      base::TimeDelta* ping_delay = nullptr) const override;
  virtual bool HasPrivilege(scada::Privilege privilege) const override;
  virtual bool IsScada() const override;
  virtual scada::NodeId GetUserId() const override;
  virtual std::string GetHostName() const override;
  virtual boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override;
  virtual scada::SessionDebugger* GetSessionDebugger() override;

  // scada::NodeManagementService
  virtual void AddNodes(const std::vector<scada::AddNodesItem>& inputs,
                        const scada::AddNodesCallback& callback) override;
  virtual void DeleteNodes(const std::vector<scada::DeleteNodesItem>& inputs,
                           const scada::DeleteNodesCallback& callback) override;
  virtual void AddReferences(
      const std::vector<scada::AddReferencesItem>& inputs,
      const scada::AddReferencesCallback& callback) override;
  virtual void DeleteReferences(
      const std::vector<scada::DeleteReferencesItem>& inputs,
      const scada::DeleteReferencesCallback& callback) override;

  // scada::ViewService
  virtual void Browse(const scada::ServiceContext& context,
                      const std::vector<scada::BrowseDescription>& nodes,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

  // scada::MonitoredItemService
  virtual std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id,
      const scada::MonitoringParameters& params) override;

  // scada::AttributeService
  virtual void Read(
      const scada::ServiceContext& context,
      const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
      const scada::ReadCallback& callback) override;
  virtual void Write(
      const scada::ServiceContext& context,
      const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
      const scada::WriteCallback& callback) override;

  // scada::MethodService
  virtual void Call(const scada::NodeId& node_id,
                    const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) override;

  // scada::HistoryService
  virtual void HistoryReadRaw(
      const scada::HistoryReadRawDetails& details,
      const scada::HistoryReadRawCallback& callback) override;
  virtual void HistoryReadEvents(
      const scada::NodeId& node_id,
      base::Time from,
      base::Time to,
      const scada::EventFilter& filter,
      const scada::HistoryReadEventsCallback& callback) override;

  // scada::CoroutineNodeManagementService
  [[nodiscard]] virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::AddNodesResult>>>
  AddNodes(std::vector<scada::AddNodesItem> inputs) override;
  [[nodiscard]] virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::StatusCode>>>
  DeleteNodes(std::vector<scada::DeleteNodesItem> inputs) override;
  [[nodiscard]] virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::StatusCode>>>
  AddReferences(std::vector<scada::AddReferencesItem> inputs) override;
  [[nodiscard]] virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::StatusCode>>>
  DeleteReferences(std::vector<scada::DeleteReferencesItem> inputs) override;

  // scada::CoroutineViewService
  [[nodiscard]] virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::BrowseResult>>>
  Browse(scada::ServiceContext context,
         std::vector<scada::BrowseDescription> inputs) override;
  [[nodiscard]] virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override;

  // scada::CoroutineAttributeService
  [[nodiscard]] virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::DataValue>>>
  Read(scada::ServiceContext context,
       std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) override;
  [[nodiscard]] virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::StatusCode>>>
  Write(scada::ServiceContext context,
        std::shared_ptr<const std::vector<scada::WriteValue>> inputs) override;

  // scada::CoroutineMethodService
  [[nodiscard]] virtual Awaitable<scada::Status> Call(
      scada::NodeId node_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments,
      scada::NodeId user_id) override;

  // scada::CoroutineHistoryService
  [[nodiscard]] virtual Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override;
  [[nodiscard]] virtual Awaitable<scada::HistoryReadEventsResult>
  HistoryReadEvents(scada::NodeId node_id,
                    base::Time from,
                    base::Time to,
                    scada::EventFilter filter) override;

 private:
  class MasterMonitoredItem;

  void ResetCoroutineAdapters();
  void RefreshCoroutineServices();
  [[nodiscard]] Awaitable<void> ConnectCoroutine(
      scada::SessionConnectParams params);
  [[nodiscard]] Awaitable<void> DisconnectCoroutine();
  [[nodiscard]] Awaitable<void> ReconnectCoroutine();

  std::vector<MasterMonitoredItem*> monitored_items_;

  boost::signals2::scoped_connection session_state_changed_connection_;

  boost::signals2::signal<void(bool connected, const scada::Status& status)>
      session_state_changed_signal_;

  DataServices services_;
  bool connected_ = false;

  std::optional<AnyExecutor> coroutine_executor_;

  std::unique_ptr<scada::CallbackToCoroutineAttributeServiceAdapter>
      attribute_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineViewServiceAdapter>
      view_service_adapter_;
  std::unique_ptr<scada::PromiseToCoroutineSessionServiceAdapter>
      session_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineMethodServiceAdapter>
      method_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineHistoryServiceAdapter>
      history_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineNodeManagementServiceAdapter>
      node_management_service_adapter_;

  scada::CoroutineAttributeService* coroutine_attribute_service_ = nullptr;
  scada::CoroutineViewService* coroutine_view_service_ = nullptr;
  scada::CoroutineSessionService* coroutine_session_service_ = nullptr;
  scada::CoroutineMethodService* coroutine_method_service_ = nullptr;
  scada::CoroutineHistoryService* coroutine_history_service_ = nullptr;
  scada::CoroutineNodeManagementService* coroutine_node_management_service_ =
      nullptr;
};
