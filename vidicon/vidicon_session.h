#pragma once

#include "address_space/address_space_impl2.h"
#include "address_space/attribute_service_impl.h"
#include "address_space/view_service_impl.h"
#include "core/attribute_service.h"
#include "core/event_service.h"
#include "core/history_service.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"
#include "core/node_management_service.h"
#include "core/session_service.h"
#include "core/view_service.h"
#include "vidicon/TeleClient.h"

#include <wrl/client.h>

namespace scada {
class ViewService;
}

class VidiconSession : public scada::SessionService,
                       public scada::HistoryService,
                       public scada::MonitoredItemService,
                       public scada::AttributeService,
                       public scada::EventService,
                       public scada::MethodService,
                       public scada::NodeManagementService,
                       public scada::ViewService {
 public:
  VidiconSession();
  ~VidiconSession();

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
  virtual bool IsScada() const override { return false; }
  virtual scada::NodeId GetUserId() const override;
  virtual std::string GetHostName() const override;
  virtual void AddObserver(scada::SessionStateObserver& observer) override;
  virtual void RemoveObserver(scada::SessionStateObserver& observer) override;

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

  // scada::MonitoredItemService
  virtual std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id,
      const scada::MonitoringParameters& params) override;

  // scada::AttributeService
  virtual void Read(const std::vector<scada::ReadValueId>& nodes,
                    const scada::ReadCallback& callback) override;
  virtual void Write(const scada::WriteValue& value,
                     const scada::NodeId& user_id,
                     const scada::StatusCallback& callback) override;

  // scada::EventService
  virtual void Acknowledge(int acknowledge_id,
                           const scada::NodeId& user_node_id) override;

  // scada::MethodService
  virtual void Call(const scada::NodeId& node_id,
                    const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) override;

  // scada::NodeManagementService
  virtual void CreateNode(const scada::NodeId& requested_id,
                          const scada::NodeId& parent_id,
                          scada::NodeClass node_class,
                          const scada::NodeId& type_id,
                          scada::NodeAttributes attributes,
                          const scada::CreateNodeCallback& callback) override;
  virtual void ModifyNodes(
      const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>&
          attributes,
      const scada::ModifyNodesCallback& callback) override;
  virtual void DeleteNode(const scada::NodeId& node_id,
                          bool return_dependencies,
                          const scada::DeleteNodeCallback& callback) override;
  virtual void ChangeUserPassword(
      const scada::NodeId& user_node_id,
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
  virtual void TranslateBrowsePaths(
      const std::vector<scada::BrowsePath>& browse_paths,
      const scada::TranslateBrowsePathCallback& callback) override;

 private:
  AddressSpaceImpl2 address_space_;
  AttributeServiceImpl attribute_service_;
  ViewServiceImpl view_service_;

  Microsoft::WRL::ComPtr<IClient> teleclient_;
};
