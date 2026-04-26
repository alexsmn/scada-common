#pragma once

#include "address_space/address_space_impl.h"
#include "address_space/attribute_service_impl.h"
#include "address_space/standard_address_space.h"
#include "address_space/view_service_impl.h"
#include "scada/attribute_service.h"
#include "scada/coroutine_services.h"
#include "scada/history_service.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "scada/node_management_service.h"
#include "scada/session_service.h"
#include "scada/view_service.h"

#include <TeleClient.h>
#include <memory>
#include <wrl/client.h>

namespace scada {
class ViewService;
}

class VidiconSession final : public scada::SessionService,
                             public scada::HistoryService,
                             public scada::MonitoredItemService,
                             public scada::AttributeService,
                             public scada::MethodService,
                             public scada::NodeManagementService,
                             public scada::ViewService,
                             public scada::CoroutineHistoryService,
                             public scada::CoroutineAttributeService,
                             public scada::CoroutineMethodService,
                             public scada::CoroutineNodeManagementService,
                             public scada::CoroutineViewService {
 public:
  VidiconSession();
  ~VidiconSession();

  // scada::SessionService
  virtual promise<void> Connect(
      const scada::SessionConnectParams& params) override;
  virtual promise<void> Disconnect() override;
  virtual promise<void> Reconnect() override;
  virtual bool IsConnected(
      base::TimeDelta* ping_delay = nullptr) const override;
  virtual bool HasPrivilege(scada::Privilege privilege) const override;
  virtual bool IsScada() const override { return false; }
  virtual scada::NodeId GetUserId() const override;
  virtual std::string GetHostName() const override;
  virtual boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override;
  virtual scada::SessionDebugger* GetSessionDebugger() override;

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

  // scada::CoroutineHistoryService
  virtual Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override;
  virtual Awaitable<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId node_id,
      base::Time from,
      base::Time to,
      scada::EventFilter filter) override;

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

  // scada::CoroutineAttributeService
  virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::DataValue>>>
  Read(scada::ServiceContext context,
       std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) override;
  virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::StatusCode>>>
  Write(scada::ServiceContext context,
        std::shared_ptr<const std::vector<scada::WriteValue>> inputs) override;

  // scada::MethodService
  virtual void Call(const scada::NodeId& node_id,
                    const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) override;

  // scada::CoroutineMethodService
  virtual Awaitable<scada::Status> Call(
      scada::NodeId node_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments,
      scada::NodeId user_id) override;

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

  // scada::CoroutineNodeManagementService
  virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::AddNodesResult>>>
  AddNodes(std::vector<scada::AddNodesItem> inputs) override;
  virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::StatusCode>>>
  DeleteNodes(std::vector<scada::DeleteNodesItem> inputs) override;
  virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::StatusCode>>>
  AddReferences(std::vector<scada::AddReferencesItem> inputs) override;
  virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::StatusCode>>>
  DeleteReferences(std::vector<scada::DeleteReferencesItem> inputs) override;

  // scada::ViewService
  virtual void Browse(
      const scada::ServiceContext& context,
      const std::vector<scada::BrowseDescription>& nodes,
      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

  // scada::CoroutineViewService
  virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::BrowseResult>>>
  Browse(scada::ServiceContext context,
         std::vector<scada::BrowseDescription> inputs) override;
  virtual Awaitable<
      std::tuple<scada::Status, std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override;

  [[nodiscard]] scada::CoroutineSessionService& coroutine_session_service();

 private:
  class CoroutineSessionAdapter;

  AddressSpaceImpl address_space_;
  StandardAddressSpace standard_address_space_{address_space_};
  SyncAttributeServiceImpl sync_attribute_service_;
  AttributeServiceImpl attribute_service_{sync_attribute_service_};
  SyncViewServiceImpl sync_view_service_;
  ViewServiceImpl view_service_{sync_view_service_};

  Microsoft::WRL::ComPtr<IClient> teleclient_;
  std::unique_ptr<CoroutineSessionAdapter> coroutine_session_service_;
};
