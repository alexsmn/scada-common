#pragma once

#include "base/time/time.h"
#include "opcua_ws/opcua_ws_message.h"
#include "opcua_ws/opcua_ws_subscription.h"

#include "scada/monitored_item_service.h"
#include "scada/service_context.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace opcua_ws {

struct OpcUaWsSessionContext {
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  scada::ServiceContext service_context;
  scada::MonitoredItemService& monitored_item_service;
  std::function<base::Time()> now = &base::Time::Now;
};

class OpcUaWsSession : private OpcUaWsSessionContext {
 public:
  explicit OpcUaWsSession(OpcUaWsSessionContext&& context);

  const scada::NodeId& GetSessionId() const {
    return this->session_id;
  }
  const scada::NodeId& GetAuthenticationToken() const {
    return this->authentication_token;
  }
  const scada::ServiceContext& GetServiceContext() const {
    return this->service_context;
  }

  OpcUaWsCreateSubscriptionResponse CreateSubscription(
      const OpcUaWsCreateSubscriptionRequest& request);
  OpcUaWsModifySubscriptionResponse ModifySubscription(
      const OpcUaWsModifySubscriptionRequest& request);
  OpcUaWsSetPublishingModeResponse SetPublishingMode(
      const OpcUaWsSetPublishingModeRequest& request);
  OpcUaWsDeleteSubscriptionsResponse DeleteSubscriptions(
      const OpcUaWsDeleteSubscriptionsRequest& request);
  OpcUaWsTransferSubscriptionsResponse TransferSubscriptionsFrom(
      OpcUaWsSession& source,
      const OpcUaWsTransferSubscriptionsRequest& request);

  OpcUaWsCreateMonitoredItemsResponse CreateMonitoredItems(
      const OpcUaWsCreateMonitoredItemsRequest& request);
  OpcUaWsModifyMonitoredItemsResponse ModifyMonitoredItems(
      const OpcUaWsModifyMonitoredItemsRequest& request);
  OpcUaWsDeleteMonitoredItemsResponse DeleteMonitoredItems(
      const OpcUaWsDeleteMonitoredItemsRequest& request);
  OpcUaWsSetMonitoringModeResponse SetMonitoringMode(
      const OpcUaWsSetMonitoringModeRequest& request);

  OpcUaWsPublishResponse Publish(const OpcUaWsPublishRequest& request);
  OpcUaWsRepublishResponse Republish(const OpcUaWsRepublishRequest& request) const;

 private:
  using SubscriptionMap =
      std::unordered_map<OpcUaWsSubscriptionId, std::unique_ptr<OpcUaWsSubscription>>;

  base::Time Now() const { return this->now(); }
  OpcUaWsSubscription* FindSubscription(OpcUaWsSubscriptionId subscription_id);
  const OpcUaWsSubscription* FindSubscription(
      OpcUaWsSubscriptionId subscription_id) const;
  void EraseSubscription(OpcUaWsSubscriptionId subscription_id);
  void AdvancePublishCursorAfter(size_t index);
  size_t FindNextReadySubscription(base::Time now, bool require_pending) const;
  void RefreshNextSubscriptionId();

  SubscriptionMap subscriptions_;
  std::vector<OpcUaWsSubscriptionId> publish_order_;
  OpcUaWsSubscriptionId next_subscription_id_ = 1;
  size_t next_publish_index_ = 0;
};

}  // namespace opcua_ws
