#pragma once

#include "opcua_ws/opcua_ws_message.h"

#include "scada/monitored_item_service.h"

#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>

namespace opcua_ws {

class OpcUaWsSubscription {
 public:
  OpcUaWsSubscription(OpcUaWsSubscriptionId subscription_id,
                      OpcUaWsSubscriptionParameters parameters,
                      scada::MonitoredItemService& monitored_item_service);

  OpcUaWsSubscription(const OpcUaWsSubscription&) = delete;
  OpcUaWsSubscription& operator=(const OpcUaWsSubscription&) = delete;

  OpcUaWsSubscriptionId subscription_id() const { return subscription_id_; }
  const OpcUaWsSubscriptionParameters& parameters() const { return parameters_; }
  bool HasPendingNotifications() const {
    return !pending_notifications_.empty();
  }
  bool IsPublishReady(base::Time now) const;
  void PrimePublishCycle(base::Time now);
  std::optional<base::Time> NextPublishDeadline() const;

  OpcUaWsModifySubscriptionResponse Modify(
      const OpcUaWsModifySubscriptionRequest& request);
  void SetPublishingEnabled(bool publishing_enabled);

  OpcUaWsCreateMonitoredItemsResponse CreateMonitoredItems(
      const OpcUaWsCreateMonitoredItemsRequest& request);
  OpcUaWsModifyMonitoredItemsResponse ModifyMonitoredItems(
      const OpcUaWsModifyMonitoredItemsRequest& request);
  OpcUaWsDeleteMonitoredItemsResponse DeleteMonitoredItems(
      const OpcUaWsDeleteMonitoredItemsRequest& request);
  OpcUaWsSetMonitoringModeResponse SetMonitoringMode(
      const OpcUaWsSetMonitoringModeRequest& request);

  std::vector<scada::StatusCode> Acknowledge(
      const std::vector<scada::UInt32>& sequence_numbers);
  std::optional<OpcUaWsPublishResponse> TryPublish(base::Time now);
  OpcUaWsRepublishResponse Republish(scada::UInt32 sequence_number) const;

 private:
  struct Item {
    OpcUaWsMonitoredItemId monitored_item_id = 0;
    scada::ReadValueId item_to_monitor;
    std::optional<std::string> index_range;
    OpcUaWsMonitoringMode monitoring_mode = OpcUaWsMonitoringMode::Reporting;
    OpcUaWsMonitoringParameters parameters;
    std::vector<std::vector<std::string>> event_field_paths;
    scada::StatusCode monitored_item_status = scada::StatusCode::Bad;
    std::shared_ptr<scada::MonitoredItem> monitored_item;
    scada::UInt32 binding_generation = 0;
  };

  struct QueuedNotification {
    OpcUaWsMonitoredItemId source_item_id = 0;
    OpcUaWsNotificationData notification;
  };

  static scada::MonitoringParameters ToMonitoringParameters(
      const Item& item,
      const OpcUaWsMonitoringParameters& parameters);

  scada::StatusCode Acknowledge(scada::UInt32 sequence_number);
  std::vector<scada::UInt32> AvailableSequenceNumbers() const;
  base::TimeDelta PublishingInterval() const;
  base::TimeDelta KeepAliveInterval() const;
  bool IsKeepAliveDue(base::Time now) const;

  void RebindItem(Item& item);
  void QueueDataChange(Item& item, const scada::DataValue& data_value);
  void QueueEvent(Item& item, const scada::Status& status, const std::any& event);
  void QueueNotification(Item& item, OpcUaWsNotificationData notification);
  void EnforceQueueLimit(const Item& item);

  static std::vector<std::vector<std::string>> ParseEventFieldPaths(
      const std::optional<OpcUaWsMonitoringFilter>& filter);
  static std::vector<scada::Variant> BuildEventFields(
      const std::vector<std::vector<std::string>>& field_paths,
      const std::any& event);

  OpcUaWsSubscriptionId subscription_id_;
  OpcUaWsSubscriptionParameters parameters_;
  scada::MonitoredItemService& monitored_item_service_;

  scada::UInt32 next_monitored_item_id_ = 1;
  scada::UInt32 next_sequence_number_ = 1;

  std::optional<base::Time> last_publish_time_;

  std::unordered_map<OpcUaWsMonitoredItemId, std::shared_ptr<Item>> items_;
  std::deque<QueuedNotification> pending_notifications_;
  std::deque<OpcUaWsNotificationMessage> retransmit_queue_;
};

}  // namespace opcua_ws
