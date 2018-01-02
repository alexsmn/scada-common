#pragma once

#include "base/win/scoped_comptr.h"
#include "core/session_service.h"
#include "core/history_service.h"
#include "core/monitored_item_service.h"
#include "core/attribute_service.h"
#include "core/event_service.h"
#include "core/node_management_service.h"
#include "core/method_service.h"
#include "core/view_service.h"
#include "vidicon/TeleClient.h"

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
  virtual void Connect(const std::string& connection_string, const std::string& username,
                       const std::string& password, bool allow_remote_logoff,
                       ConnectCallback callback) override;
  virtual bool IsConnected() const override;
  virtual bool IsAdministrator() const override;
  virtual bool IsScada() const override { return false; }
  virtual scada::NodeId GetUserId() const override;
  virtual std::string GetHostName() const override;
  virtual void AddObserver(scada::SessionStateObserver& observer) override;
  virtual void RemoveObserver(scada::SessionStateObserver& observer) override;

  // scada::HistoryService
  virtual void QueryItemInfos(const scada::ItemInfosCallback& callback) override;
  virtual void WriteItemInfo(const scada::ItemInfo& info) override;
  virtual void HistoryRead(const scada::ReadValueId& read_value_id, base::Time from, base::Time to,
                           const scada::Filter& filter, const scada::HistoryReadCallback& callback) override;
  virtual void WriteEvent(const scada::Event& event) override;
  virtual void AcknowledgeEvent(unsigned ack_id, base::Time time,
                                const scada::NodeId& user_node_id) override;

  // scada::MonitoredItemService
  virtual std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(const scada::ReadValueId& read_value_id) override;

  // scada::AttributeService
  virtual void Read(const std::vector<scada::ReadValueId>& nodes, const scada::ReadCallback& callback) override;
  virtual void Write(const scada::NodeId& node_id, double value, const scada::NodeId& user_id,
                     const scada::WriteFlags& flags, const StatusCallback& callback) override;

  // scada::EventService
  virtual void Acknowledge(int acknowledge_id, const scada::NodeId& user_node_id) override;
  virtual void GenerateEvent(const scada::Event& event) override;

  // scada::MethodService
  virtual void Call(const scada::NodeId& node_id, const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const StatusCallback& callback) override;

  // scada::NodeManagementService
  virtual void CreateNode(const scada::NodeId& requested_id, const scada::NodeId& parent_id,
                          scada::NodeClass node_class, const scada::NodeId& type_id, scada::NodeAttributes attributes,
                          const CreateNodeCallback& callback) override;
  virtual void ModifyNodes(const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>& attributes,
                           const MultiStatusCallback& callback) override;
  virtual void DeleteNode(const scada::NodeId& node_id, bool return_relations,
                          const DeleteNodeCallback& callback) override;
  virtual void ChangeUserPassword(const scada::NodeId& user_node_id,
                                  const std::string& current_password,
                                  const std::string& new_password,
                                  const StatusCallback& callback) override;
  virtual void AddReference(const scada::NodeId& reference_type_id, const scada::NodeId& source_id, const scada::NodeId& target_id,
                            const StatusCallback& callback) override;
  virtual void DeleteReference(const scada::NodeId& reference_type_id, const scada::NodeId& source_id, const scada::NodeId& target_id,
                               const StatusCallback& callback) override;

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& nodes, const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePath(const scada::NodeId& starting_node_id, const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback) override;
  virtual void Subscribe(scada::ViewEvents& events) override;
  virtual void Unsubscribe(scada::ViewEvents& events) override;

 private:
  base::win::ScopedComPtr<IClient> teleclient_;
};
