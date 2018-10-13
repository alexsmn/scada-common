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
                                 private scada::SessionStateObserver,
                                 private scada::ViewEvents {
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
  virtual void CreateNode(const scada::NodeId& requested_id,
                          const scada::NodeId& parent_id,
                          scada::NodeClass node_class,
                          const scada::NodeId& type_id,
                          scada::NodeAttributes attributes,
                          const CreateNodeCallback& callback) override;
  virtual void ModifyNodes(
      const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>&
          attributes,
      const scada::ModifyNodesCallback& callback) override;
  virtual void DeleteNode(const scada::NodeId& node_id,
                          bool return_dependencies,
                          const scada::DeleteNodeCallback& callback) override;
  virtual void ChangeUserPassword(
      const scada::NodeId& user_id,
      const scada::LocalizedText& current_password,
      const scada::LocalizedText& new_password,
      const scada::StatusCallback& callback) override;
  virtual void AddReference(const scada::NodeId& reference_type_id,
                            const scada::NodeId& source_id,
                            const scada::NodeId& target_id,
                            const scada::StatusCallback& callback) override;
  virtual void DeleteReference(const scada::NodeId& reference_type_id,
                               const scada::NodeId& source_id,
                               const scada::NodeId& target_id,
                               const scada::StatusCallback& callback) override;

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& nodes,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePath(
      const scada::NodeId& starting_node_id,
      const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback) override;
  virtual void Subscribe(scada::ViewEvents& events) override;
  virtual void Unsubscribe(scada::ViewEvents& events) override;

  // scada::EventService
  virtual void Acknowledge(int acknowledge_id,
                           const scada::NodeId& user_node_id) override;

  // scada::MonitoredItemService
  virtual std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id) override;

  // scada::AttributeService
  virtual void Read(const std::vector<scada::ReadValueId>& nodes,
                    const scada::ReadCallback& callback) override;
  virtual void Write(const scada::WriteValue& value,
                     const scada::NodeId& user_id,
                     const scada::StatusCallback& callback) override;

  // scada::MethodService
  virtual void Call(const scada::NodeId& node_id,
                    const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) override;

  // scada::HistoryService
  virtual void HistoryReadRaw(
      const scada::NodeId& node_id,
      base::Time from,
      base::Time to,
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

  // scada::ViewEvents
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(const scada::NodeId& node_id) override;

  std::vector<MasterMonitoredItem*> monitored_items_;

  base::ObserverList<scada::SessionStateObserver> session_state_observers_;
  base::ObserverList<scada::ViewEvents> view_events_;

  DataServices services_;
  bool connected_ = false;
};
