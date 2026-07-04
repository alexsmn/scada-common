#pragma once

#include "address_space/address_space_impl.h"
#include "address_space/attribute_service_impl.h"
#include "address_space/standard_address_space.h"
#include "address_space/view_service_impl.h"
#include "scada/attribute_service.h"
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
                             public scada::ViewService,
                             public scada::NodeManagementService {
 public:
  VidiconSession();
  ~VidiconSession();

  // scada::SessionService
  virtual Awaitable<void> Connect(scada::SessionConnectParams params) override;
  virtual Awaitable<scada::Status> ConnectStatus(
      scada::SessionConnectParams params) override;
  virtual Awaitable<void> Disconnect() override;
  virtual Awaitable<void> Reconnect() override;
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
  virtual Awaitable<scada::HistoryReadRawResult> HistoryReadRaw(
      scada::HistoryReadRawDetails details) override;
  virtual Awaitable<scada::HistoryReadEventsResult> HistoryReadEvents(
      scada::NodeId node_id,
      base::Time from,
      base::Time to,
      scada::EventFilter filter) override;

  // scada::MonitoredItemService
  scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
  CreateSubscription(scada::ServiceContext context,
                     scada::MonitoredItemSubscriptionOptions options) override;

  // scada::AttributeService
  virtual Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> Read(
      scada::ServiceContext context,
      std::vector<scada::ReadValueId> inputs) override;
  virtual Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> Write(
      scada::ServiceContext context,
      std::vector<scada::WriteValue> inputs) override;

  // scada::MethodService
  virtual Awaitable<scada::Status> Call(scada::NodeId node_id,
                                        scada::NodeId method_id,
                                        std::vector<scada::Variant> arguments,
                                        scada::ServiceContext context) override;

  // scada::NodeManagementService
  virtual Awaitable<scada::StatusOr<std::vector<scada::AddNodesResult>>>
  AddNodes(scada::ServiceContext context,
           std::vector<scada::AddNodesItem> inputs) override;
  virtual Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
  DeleteNodes(scada::ServiceContext context,
              std::vector<scada::DeleteNodesItem> inputs) override;
  virtual Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
  AddReferences(scada::ServiceContext context,
                std::vector<scada::AddReferencesItem> inputs) override;
  virtual Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
  DeleteReferences(scada::ServiceContext context,
                   std::vector<scada::DeleteReferencesItem> inputs) override;

  // scada::ViewService
  virtual Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>> Browse(
      scada::ServiceContext context,
      std::vector<scada::BrowseDescription> inputs) override;
  virtual Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) override;

 private:
  AddressSpaceImpl address_space_;
  StandardAddressSpace standard_address_space_{address_space_};
  SyncAttributeServiceImpl sync_attribute_service_;
  AttributeServiceImpl attribute_service_{sync_attribute_service_};
  SyncViewServiceImpl sync_view_service_;
  ViewServiceImpl view_service_{sync_view_service_};

  Microsoft::WRL::ComPtr<IClient> teleclient_;
};
