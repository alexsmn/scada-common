#pragma once

#include "base/time/time.h"
#include "opcua/message.h"
#include "opcua/service_message.h"
#include "opcua/server_subscription.h"

#include "scada/monitored_item_service.h"
#include "scada/service_context.h"

#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace opcua {

struct OpcUaServerSessionContext {
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  scada::ServiceContext service_context;
  scada::MonitoredItemService& monitored_item_service;
  std::function<base::Time()> now = &base::Time::Now;
};

class OpcUaServerSession : private OpcUaServerSessionContext {
 public:
  struct PublishPollResult {
    std::optional<OpcUaPublishResponse> response;
    std::optional<base::TimeDelta> wait_for;
  };

  explicit OpcUaServerSession(OpcUaServerSessionContext&& context);

  const scada::NodeId& GetSessionId() const {
    return this->session_id;
  }
  const scada::NodeId& GetAuthenticationToken() const {
    return this->authentication_token;
  }
  const scada::ServiceContext& GetServiceContext() const {
    return this->service_context;
  }

  OpcUaCreateSubscriptionResponse CreateSubscription(
      const OpcUaCreateSubscriptionRequest& request);
  OpcUaCreateSubscriptionResponse CreateSubscriptionWithId(
      OpcUaSubscriptionId subscription_id,
      const OpcUaCreateSubscriptionRequest& request);
  OpcUaModifySubscriptionResponse ModifySubscription(
      const OpcUaModifySubscriptionRequest& request);
  OpcUaSetPublishingModeResponse SetPublishingMode(
      const OpcUaSetPublishingModeRequest& request);
  OpcUaDeleteSubscriptionsResponse DeleteSubscriptions(
      const OpcUaDeleteSubscriptionsRequest& request);
  OpcUaTransferSubscriptionsResponse TransferSubscriptionsFrom(
      OpcUaServerSession& source,
      const OpcUaTransferSubscriptionsRequest& request);

  OpcUaCreateMonitoredItemsResponse CreateMonitoredItems(
      const OpcUaCreateMonitoredItemsRequest& request);
  OpcUaModifyMonitoredItemsResponse ModifyMonitoredItems(
      const OpcUaModifyMonitoredItemsRequest& request);
  OpcUaDeleteMonitoredItemsResponse DeleteMonitoredItems(
      const OpcUaDeleteMonitoredItemsRequest& request);
  OpcUaSetMonitoringModeResponse SetMonitoringMode(
      const OpcUaSetMonitoringModeRequest& request);

  std::vector<scada::StatusCode> AcknowledgePublishRequest(
      const OpcUaPublishRequest& request);
  PublishPollResult PollPublish();
  OpcUaPublishResponse Publish(const OpcUaPublishRequest& request);
  OpcUaRepublishResponse Republish(const OpcUaRepublishRequest& request) const;
  BrowseResponse StoreBrowseResults(
      BrowseResponse response,
      size_t requested_max_references_per_node);
  BrowseNextResponse BrowseNext(const BrowseNextRequest& request);
  std::vector<OpcUaSubscriptionId> GetSubscriptionIds() const;
  bool HasSubscription(OpcUaSubscriptionId subscription_id) const;

 private:
  struct ByteStringHash {
    size_t operator()(const scada::ByteString& value) const;
  };

  struct BrowseContinuationState {
    std::vector<scada::ReferenceDescription> remaining_references;
  };

  using SubscriptionMap =
      std::unordered_map<OpcUaSubscriptionId,
                         std::unique_ptr<OpcUaServerSubscription>>;
  using BrowseContinuationMap =
      std::unordered_map<scada::ByteString, BrowseContinuationState, ByteStringHash>;

  base::Time Now() const { return this->now(); }
  OpcUaServerSubscription* FindSubscription(OpcUaSubscriptionId subscription_id);
  const OpcUaServerSubscription* FindSubscription(
      OpcUaSubscriptionId subscription_id) const;
  void EraseSubscription(OpcUaSubscriptionId subscription_id);
  void AdvancePublishCursorAfter(size_t index);
  size_t FindNextReadySubscription(base::Time now, bool require_pending) const;
  void RefreshNextSubscriptionId();
  scada::ByteString MakeBrowseContinuationPoint();
  scada::BrowseResult PageBrowseResult(
      scada::BrowseResult result,
      size_t requested_max_references_per_node);
  scada::BrowseResult ResumeBrowseResult(
      const scada::ByteString& continuation_point);

  SubscriptionMap subscriptions_;
  BrowseContinuationMap browse_continuations_;
  std::vector<OpcUaSubscriptionId> publish_order_;
  OpcUaSubscriptionId next_subscription_id_ = 1;
  size_t next_publish_index_ = 0;
  scada::UInt32 next_browse_continuation_id_ = 1;
};

}  // namespace opcua
