#pragma once

#include "opcua/opcua_message.h"

#include "scada/monitored_item_service.h"

#include <deque>
#include <memory>
#include <optional>
#include <unordered_map>

namespace opcua {

class OpcUaSubscription {
 public:
  OpcUaSubscription(OpcUaSubscriptionId subscription_id,
                    OpcUaSubscriptionParameters parameters,
                    scada::MonitoredItemService& monitored_item_service,
                    base::Time publish_cycle_start_time);

  OpcUaSubscription(const OpcUaSubscription&) = delete;
  OpcUaSubscription& operator=(const OpcUaSubscription&) = delete;

  OpcUaSubscriptionId subscription_id() const { return subscription_id_; }
  const OpcUaSubscriptionParameters& parameters() const { return parameters_; }
  bool HasPendingNotifications() const {
    return !pending_notifications_.empty();
  }
  base::TimeDelta PublishingInterval() const;
  bool IsPublishReady(base::Time now) const;
  void PrimePublishCycle(base::Time now);
  std::optional<base::Time> NextPublishDeadline() const;

  OpcUaModifySubscriptionResponse Modify(
      const OpcUaModifySubscriptionRequest& request);
  void SetPublishingEnabled(bool publishing_enabled);

  OpcUaCreateMonitoredItemsResponse CreateMonitoredItems(
      const OpcUaCreateMonitoredItemsRequest& request);
  OpcUaModifyMonitoredItemsResponse ModifyMonitoredItems(
      const OpcUaModifyMonitoredItemsRequest& request);
  OpcUaDeleteMonitoredItemsResponse DeleteMonitoredItems(
      const OpcUaDeleteMonitoredItemsRequest& request);
  OpcUaSetMonitoringModeResponse SetMonitoringMode(
      const OpcUaSetMonitoringModeRequest& request);

  std::vector<scada::StatusCode> Acknowledge(
      const std::vector<scada::UInt32>& sequence_numbers);
  std::optional<OpcUaPublishResponse> TryPublish(base::Time now);
  OpcUaRepublishResponse Republish(scada::UInt32 sequence_number) const;

 private:
  struct Item {
    OpcUaMonitoredItemId monitored_item_id = 0;
    scada::ReadValueId item_to_monitor;
    std::optional<std::string> index_range;
    OpcUaMonitoringMode monitoring_mode = OpcUaMonitoringMode::Reporting;
    OpcUaMonitoringParameters parameters;
    std::vector<std::vector<std::string>> event_field_paths;
    scada::StatusCode monitored_item_status = scada::StatusCode::Bad;
    std::shared_ptr<scada::MonitoredItem> monitored_item;
    scada::UInt32 binding_generation = 0;
  };

  struct QueuedNotification {
    OpcUaMonitoredItemId source_item_id = 0;
    OpcUaNotificationData notification;
  };

  static scada::MonitoringParameters ToMonitoringParameters(
      const Item& item,
      const OpcUaMonitoringParameters& parameters);

  scada::StatusCode Acknowledge(scada::UInt32 sequence_number);
  std::vector<scada::UInt32> AvailableSequenceNumbers() const;
  base::TimeDelta KeepAliveInterval() const;
  bool IsKeepAliveDue(base::Time now) const;

  void RebindItem(Item& item);
  void QueueDataChange(Item& item, const scada::DataValue& data_value);
  void QueueEvent(Item& item, const scada::Status& status, const std::any& event);
  void QueueNotification(Item& item, OpcUaNotificationData notification);
  void EnforceQueueLimit(const Item& item);

  static std::vector<std::vector<std::string>> ParseEventFieldPaths(
      const std::optional<OpcUaMonitoringFilter>& filter);
  static std::vector<scada::Variant> BuildEventFields(
      const std::vector<std::vector<std::string>>& field_paths,
      const std::any& event);

  OpcUaSubscriptionId subscription_id_;
  OpcUaSubscriptionParameters parameters_;
  scada::MonitoredItemService& monitored_item_service_;

  scada::UInt32 next_monitored_item_id_ = 1;
  scada::UInt32 next_sequence_number_ = 1;

  bool initial_message_sent_ = false;
  std::optional<base::Time> last_publish_time_;

  std::unordered_map<OpcUaMonitoredItemId, std::shared_ptr<Item>> items_;
  std::deque<QueuedNotification> pending_notifications_;
  std::deque<OpcUaNotificationMessage> retransmit_queue_;
};

}  // namespace opcua

