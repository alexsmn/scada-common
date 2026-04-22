#pragma once

#include "base/time/time.h"
#include "opcua_ws/opcua_ws_message.h"
#include "opcua_ws/opcua_ws_subscription.h"

#include "scada/monitored_item_service.h"
#include "scada/service_context.h"

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace opcua {

using opcua_ws::BrowseNextRequest;
using opcua_ws::BrowseNextResponse;
using opcua_ws::BrowseResponse;
using opcua_ws::OpcUaWsCreateMonitoredItemsRequest;
using opcua_ws::OpcUaWsCreateMonitoredItemsResponse;
using opcua_ws::OpcUaWsCreateSubscriptionRequest;
using opcua_ws::OpcUaWsCreateSubscriptionResponse;
using opcua_ws::OpcUaWsDeleteMonitoredItemsRequest;
using opcua_ws::OpcUaWsDeleteMonitoredItemsResponse;
using opcua_ws::OpcUaWsDeleteSubscriptionsRequest;
using opcua_ws::OpcUaWsDeleteSubscriptionsResponse;
using opcua_ws::OpcUaWsModifyMonitoredItemsRequest;
using opcua_ws::OpcUaWsModifyMonitoredItemsResponse;
using opcua_ws::OpcUaWsModifySubscriptionRequest;
using opcua_ws::OpcUaWsModifySubscriptionResponse;
using opcua_ws::OpcUaWsPublishRequest;
using opcua_ws::OpcUaWsPublishResponse;
using opcua_ws::OpcUaWsRepublishRequest;
using opcua_ws::OpcUaWsRepublishResponse;
using opcua_ws::OpcUaWsSetMonitoringModeRequest;
using opcua_ws::OpcUaWsSetMonitoringModeResponse;
using opcua_ws::OpcUaWsSetPublishingModeRequest;
using opcua_ws::OpcUaWsSetPublishingModeResponse;
using opcua_ws::OpcUaWsSubscriptionId;
using opcua_ws::OpcUaWsTransferSubscriptionsRequest;
using opcua_ws::OpcUaWsTransferSubscriptionsResponse;

struct OpcUaSessionContext {
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  scada::ServiceContext service_context;
  scada::MonitoredItemService& monitored_item_service;
  std::function<base::Time()> now = &base::Time::Now;
};

class OpcUaSession : private OpcUaSessionContext {
 public:
  struct PublishPollResult {
    std::optional<OpcUaWsPublishResponse> response;
    std::optional<base::TimeDelta> wait_for;
  };

  explicit OpcUaSession(OpcUaSessionContext&& context);

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
  OpcUaWsCreateSubscriptionResponse CreateSubscriptionWithId(
      OpcUaWsSubscriptionId subscription_id,
      const OpcUaWsCreateSubscriptionRequest& request);
  OpcUaWsModifySubscriptionResponse ModifySubscription(
      const OpcUaWsModifySubscriptionRequest& request);
  OpcUaWsSetPublishingModeResponse SetPublishingMode(
      const OpcUaWsSetPublishingModeRequest& request);
  OpcUaWsDeleteSubscriptionsResponse DeleteSubscriptions(
      const OpcUaWsDeleteSubscriptionsRequest& request);
  OpcUaWsTransferSubscriptionsResponse TransferSubscriptionsFrom(
      OpcUaSession& source,
      const OpcUaWsTransferSubscriptionsRequest& request);

  OpcUaWsCreateMonitoredItemsResponse CreateMonitoredItems(
      const OpcUaWsCreateMonitoredItemsRequest& request);
  OpcUaWsModifyMonitoredItemsResponse ModifyMonitoredItems(
      const OpcUaWsModifyMonitoredItemsRequest& request);
  OpcUaWsDeleteMonitoredItemsResponse DeleteMonitoredItems(
      const OpcUaWsDeleteMonitoredItemsRequest& request);
  OpcUaWsSetMonitoringModeResponse SetMonitoringMode(
      const OpcUaWsSetMonitoringModeRequest& request);

  std::vector<scada::StatusCode> AcknowledgePublishRequest(
      const OpcUaWsPublishRequest& request);
  PublishPollResult PollPublish();
  OpcUaWsPublishResponse Publish(const OpcUaWsPublishRequest& request);
  OpcUaWsRepublishResponse Republish(const OpcUaWsRepublishRequest& request) const;
  BrowseResponse StoreBrowseResults(BrowseResponse response,
                                    size_t requested_max_references_per_node);
  BrowseNextResponse BrowseNext(const BrowseNextRequest& request);
  std::vector<OpcUaWsSubscriptionId> GetSubscriptionIds() const;
  bool HasSubscription(OpcUaWsSubscriptionId subscription_id) const;

 private:
  struct ByteStringHash {
    size_t operator()(const scada::ByteString& value) const;
  };

  struct BrowseContinuationState {
    std::vector<scada::ReferenceDescription> remaining_references;
  };

  using SubscriptionMap =
      std::unordered_map<OpcUaWsSubscriptionId, std::unique_ptr<OpcUaSubscription>>;
  using BrowseContinuationMap =
      std::unordered_map<scada::ByteString, BrowseContinuationState, ByteStringHash>;

  base::Time Now() const { return this->now(); }
  OpcUaSubscription* FindSubscription(OpcUaWsSubscriptionId subscription_id);
  const OpcUaSubscription* FindSubscription(
      OpcUaWsSubscriptionId subscription_id) const;
  void EraseSubscription(OpcUaWsSubscriptionId subscription_id);
  void AdvancePublishCursorAfter(size_t index);
  size_t FindNextReadySubscription(base::Time now, bool require_pending) const;
  void RefreshNextSubscriptionId();
  scada::ByteString MakeBrowseContinuationPoint();
  scada::BrowseResult PageBrowseResult(scada::BrowseResult result,
                                       size_t requested_max_references_per_node);
  scada::BrowseResult ResumeBrowseResult(
      const scada::ByteString& continuation_point);

  SubscriptionMap subscriptions_;
  BrowseContinuationMap browse_continuations_;
  std::vector<OpcUaWsSubscriptionId> publish_order_;
  OpcUaWsSubscriptionId next_subscription_id_ = 1;
  size_t next_publish_index_ = 0;
  scada::UInt32 next_browse_continuation_id_ = 1;
};

}  // namespace opcua

namespace opcua_ws {

using OpcUaWsSessionContext = opcua::OpcUaSessionContext;
using OpcUaWsSession = opcua::OpcUaSession;

}  // namespace opcua_ws
