#pragma once

#include "core/monitored_item.h"

#include <map>
#include <memory>
#include <opcuapp/basic_types.h>
#include <opcuapp/client/subscription.h>
#include <vector>

namespace opcua {
namespace client {
class Session;
}
}

class OpcUaMonitoredItem;

struct OpcUaMonitoredItemCreateResult {
  opcua::StatusCode status_code;
  opcua::MonitoredItemId monitored_item_id;
  double revised_sampling_interval;
};

struct OpcUaSubscriptionContext {
  opcua::client::Session& session_;
  std::function<void(const scada::Status& status)> error_handler_;
};

class OpcUaSubscription : public std::enable_shared_from_this<OpcUaSubscription>,
                          private OpcUaSubscriptionContext {
 public:
  static std::shared_ptr<OpcUaSubscription> Create(OpcUaSubscriptionContext&& context);

  OpcUaSubscription(const OpcUaSubscription&) = delete;
  ~OpcUaSubscription();

  void Reset();

  bool created() const { return created_; }

  std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(const scada::ReadValueId& read_value_id);

 private:
  explicit OpcUaSubscription(OpcUaSubscriptionContext&& session);

  void Init();

  struct Item {
    const opcua::MonitoredItemClientHandle client_handle;
    const scada::ReadValueId read_value_id;
    scada::DataChangeHandler data_change_handler;
    scada::EventHandler event_handler;
    bool subscribed;
    bool added;
    opcua::MonitoredItemId id;
  };

  void Subscribe(opcua::MonitoredItemClientHandle client_handle, scada::ReadValueId read_value_id,
      scada::DataChangeHandler data_change_handler, scada::EventHandler event_handler);
  void Unsubscribe(opcua::MonitoredItemClientHandle client_handle);
  Item* FindItem(opcua::MonitoredItemClientHandle client_handle);
  void ScheduleCommitItems();
  void ScheduleCommitItemsDone();
  void CommitItems();

  void CreateSubscription();
  void OnCreateSubscriptionResponse(const scada::Status& status);

  void CreateMonitoredItems();
  void OnCreateMonitoredItemsResponse(const scada::Status& status, std::vector<OpcUaMonitoredItemCreateResult> results);

  void DeleteMonitoredItems();
  void OnDeleteMonitoredItemsResponse(const scada::Status& status, std::vector<scada::StatusCode> results);

  void OnDataChange(std::vector<opcua::MonitoredItemNotification> notifications);
  void OnError(const scada::Status& status);

  opcua::client::Subscription subscription_;

  bool created_ = false;
  bool commit_items_scheduled_ = false;

  std::map<opcua::MonitoredItemClientHandle, Item> items_;
  std::vector<Item*> pending_subscribe_items_;
  std::vector<Item*> subscribing_items_;
  std::vector<Item*> pending_unsubscribe_items_;
  std::vector<Item*> unsubscribing_items_;

  opcua::MonitoredItemClientHandle next_monitored_item_client_handle_ = 0;

  friend class OpcUaMonitoredItem;
};
