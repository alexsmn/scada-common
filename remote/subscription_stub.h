#pragma once

#include "remote/subscription.h"

#include <map>
#include <memory>

namespace scada {
class Event;
class MonitoredItem;
class MonitoredItemService;
struct ReadValueId;
struct MonitoringParameters;
}  // namespace scada

class MessageSender;
struct DataItemUpdate;

class SubscriptionStub {
 public:
  SubscriptionStub(MessageSender& sender,
                   scada::MonitoredItemService& monitored_item_service,
                   int subscription_id,
                   const SubscriptionParams& params);
  ~SubscriptionStub();

  void OnCreateMonitoredItem(int request_id,
                             const scada::ReadValueId& read_value_id,
                             const scada::MonitoringParameters& params);
  void OnDeleteMonitoredItem(int request_id, int monitored_item_id);

 private:
  void OnDataChange(MonitoredItemId monitored_item_id,
                    const scada::DataValue& data_value);
  void OnEvent(MonitoredItemId monitored_item_id,
               const scada::Status& status,
               const scada::Event& event);

  MessageSender& sender_;
  scada::MonitoredItemService& monitored_item_service_;
  int subscription_id_;

  MonitoredItemId next_monitored_item_id_ = 1;
  std::map<MonitoredItemId, std::unique_ptr<scada::MonitoredItem>> channels_;
};
