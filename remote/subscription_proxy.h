#pragma once

#include <map>
#include <memory>
#include <set>

#include "base/strings/string_piece.h"
#include "core/attribute_ids.h"
#include "core/status.h"
#include "remote/subscription.h"

namespace scada {
class Event;
class MonitoredItem;
struct ReadValueId;
}  // namespace scada

namespace protocol {
class Response;
}

class MessageSender;

class SubscriptionProxy {
 public:
  explicit SubscriptionProxy(const SubscriptionParams& params);
  ~SubscriptionProxy();

  std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId& read_value_id);

  void OnChannelOpened(MessageSender& sender);
  void OnChannelClosed();
  void OnDataChange(int monitored_item_id, const scada::DataValue& data_value);
  void OnEvent(int monitored_item_id,
               const scada::Status& status,
               const scada::Event& event);

 private:
  class MonitoredItemProxy;

  void AddMonitoredItem(MonitoredItemProxy& item);
  void RemoveMonitoredItem(MonitoredItemProxy& item);

  void OnCreateSubscriptionResult(const scada::Status& status,
                                  int subscription_id);

  MessageSender* sender_;
  
  std::set<MonitoredItemProxy*> monitored_items_;
  std::map<MonitoredItemId, MonitoredItemProxy*> monitored_item_ids_;

  enum State { DELETED, CREATING, CREATED };
  State state_;

  int subscription_id_;

  std::shared_ptr<bool> cancelation_;
};
