#pragma once

#include "base/observer_list.h"
#include "core/attribute_service.h"
#include "core/data_services.h"
#include "core/event_service.h"
#include "core/history_service.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"
#include "core/node_management_service.h"
#include "core/session_service.h"
#include "core/session_state_observer.h"
#include "core/view_service.h"

class MasterDataServices final : public scada::AttributeService,
                                 public scada::ViewService,
                                 public scada::SessionService,
                                 public scada::MonitoredItemService,
                                 public scada::EventService,
                                 public scada::MethodService,
                                 public scada::HistoryService,
                                 public scada::NodeManagementService,
                                 private scada::SessionStateObserver {
 public:
  MasterDataServices();
  ~MasterDataServices();

  void SetServices(DataServices&& sevices);

  // scada::SessionService
  virtual void Connect(const std::string& connection_string,
                       const scada::LocalizedText& user_name,
                       const scada::LocalizedText& password,
                       bool allow_remote_logoff,
                       const scada::StatusCallback& callback) override;
  virtual void Reconnect() override;
  virtual void Disconnect(const scada::StatusCallback& callback) override;
  virtual bool IsConnected(
      base::TimeDelta* ping_delay = nullptr) const override;
  virtual bool HasPrivilege(scada::Privilege privilege) const override;
  virtual bool IsScada() const override;
  virtual scada::NodeId GetUserId() const override;
  virtual std::string GetHostName() const override;
  virtual void AddObserver(scada::SessionStateObserver& observer) override;
  virtual void RemoveObserver(scada::SessionStateObserver& observer) override;

  // scada::NodeManagementService
  virtual void AddNodes(const std::vector<scada::AddNodesItem>& inputs,
                        const scada::AddNodesCallback& callback) override;
  virtual void DeleteNodes(const std::vector<scada::DeleteNodesItem>& inputs,
                           const scada::DeleteNodesCallback& callback) override;
  virtual void ChangeUserPassword(
      const scada::NodeId& user_id,
      const scada::LocalizedText& current_password,
      const scada::LocalizedText& new_password,
      const scada::StatusCallback& callback) override;
  virtual void AddReferences(
      const std::vector<scada::AddReferencesItem>& inputs,
      const scada::AddReferencesCallback& callback) override;
  virtual void DeleteReferences(
      const std::vector<scada::DeleteReferencesItem>& inputs,
      const scada::DeleteReferencesCallback& callback) override;

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& nodes,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathsCallback& callback) override;

  // scada::EventService
  virtual void Acknowledge(base::span<const int> acknowledge_ids,
                           const scada::NodeId& user_id) override;

  // scada::MonitoredItemService
  virtual std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id,
      const scada::MonitoringParameters& params) override;

  // scada::AttributeService
  virtual void Read(
      const std::shared_ptr<const scada::ServiceContext>& context,
      const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
      const scada::ReadCallback& callback) override;
  virtual void Write(
      const std::shared_ptr<const scada::ServiceContext>& context,
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

 private:
  class MasterMonitoredItem;

  // scada::SessionStateObserver
  virtual void OnSessionCreated() override;
  virtual void OnSessionDeleted(const scada::Status& status) override;

  std::vector<MasterMonitoredItem*> monitored_items_;

  base::ObserverList<scada::SessionStateObserver> session_state_observers_;

  DataServices services_;
  bool connected_ = false;
};
