#pragma once

#include "opcua_ws/opcua_ws_service_message.h"
#include "opcua_ws/opcua_ws_session_manager.h"

#include <boost/json/value.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace opcua_ws {

struct OpcUaWsServiceFault {
  scada::Status status{scada::StatusCode::Bad};
};

using OpcUaWsSubscriptionId = scada::UInt32;
using OpcUaWsMonitoredItemId = scada::UInt32;

enum class OpcUaWsMonitoringMode : scada::UInt32 {
  Disabled = 0,
  Sampling = 1,
  Reporting = 2,
};

enum class OpcUaWsTimestampsToReturn : scada::UInt32 {
  Source = 0,
  Server = 1,
  Both = 2,
  Neither = 3,
};

enum class OpcUaWsDeadbandType : scada::UInt32 {
  None = 0,
  Absolute = 1,
  Percent = 2,
};

enum class OpcUaWsDataChangeTrigger : scada::UInt32 {
  Status = 0,
  StatusValue = 1,
  StatusValueTimestamp = 2,
};

struct OpcUaWsDataChangeFilter {
  bool operator==(const OpcUaWsDataChangeFilter&) const = default;

  OpcUaWsDataChangeTrigger trigger = OpcUaWsDataChangeTrigger::StatusValue;
  OpcUaWsDeadbandType deadband_type = OpcUaWsDeadbandType::None;
  double deadband_value = 0;
};

using OpcUaWsMonitoringFilter =
    std::variant<OpcUaWsDataChangeFilter, boost::json::value>;

struct OpcUaWsMonitoringParameters {
  bool operator==(const OpcUaWsMonitoringParameters&) const = default;

  scada::UInt32 client_handle = 0;
  double sampling_interval_ms = 0;
  std::optional<OpcUaWsMonitoringFilter> filter;
  scada::UInt32 queue_size = 1;
  bool discard_oldest = true;
};

struct OpcUaWsMonitoredItemCreateRequest {
  bool operator==(const OpcUaWsMonitoredItemCreateRequest&) const = default;

  scada::ReadValueId item_to_monitor;
  std::optional<std::string> index_range;
  OpcUaWsMonitoringMode monitoring_mode = OpcUaWsMonitoringMode::Reporting;
  OpcUaWsMonitoringParameters requested_parameters;
};

struct OpcUaWsMonitoredItemCreateResult {
  bool operator==(const OpcUaWsMonitoredItemCreateResult&) const = default;

  scada::Status status{scada::StatusCode::Good};
  OpcUaWsMonitoredItemId monitored_item_id = 0;
  double revised_sampling_interval_ms = 0;
  scada::UInt32 revised_queue_size = 0;
  std::optional<boost::json::value> filter_result;
};

struct OpcUaWsMonitoredItemModifyRequest {
  bool operator==(const OpcUaWsMonitoredItemModifyRequest&) const = default;

  OpcUaWsMonitoredItemId monitored_item_id = 0;
  OpcUaWsMonitoringParameters requested_parameters;
};

struct OpcUaWsMonitoredItemModifyResult {
  bool operator==(const OpcUaWsMonitoredItemModifyResult&) const = default;

  scada::Status status{scada::StatusCode::Good};
  double revised_sampling_interval_ms = 0;
  scada::UInt32 revised_queue_size = 0;
  std::optional<boost::json::value> filter_result;
};

struct OpcUaWsSubscriptionParameters {
  bool operator==(const OpcUaWsSubscriptionParameters&) const = default;

  double publishing_interval_ms = 0;
  scada::UInt32 lifetime_count = 0;
  scada::UInt32 max_keep_alive_count = 0;
  scada::UInt32 max_notifications_per_publish = 0;
  bool publishing_enabled = true;
  scada::UInt8 priority = 0;
};

struct OpcUaWsCreateSubscriptionRequest {
  OpcUaWsSubscriptionParameters parameters;
};

struct OpcUaWsCreateSubscriptionResponse {
  scada::Status status{scada::StatusCode::Good};
  OpcUaWsSubscriptionId subscription_id = 0;
  double revised_publishing_interval_ms = 0;
  scada::UInt32 revised_lifetime_count = 0;
  scada::UInt32 revised_max_keep_alive_count = 0;
};

struct OpcUaWsModifySubscriptionRequest {
  OpcUaWsSubscriptionId subscription_id = 0;
  OpcUaWsSubscriptionParameters parameters;
};

struct OpcUaWsModifySubscriptionResponse {
  scada::Status status{scada::StatusCode::Good};
  double revised_publishing_interval_ms = 0;
  scada::UInt32 revised_lifetime_count = 0;
  scada::UInt32 revised_max_keep_alive_count = 0;
};

struct OpcUaWsSetPublishingModeRequest {
  bool publishing_enabled = true;
  std::vector<OpcUaWsSubscriptionId> subscription_ids;
};

struct OpcUaWsSetPublishingModeResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct OpcUaWsDeleteSubscriptionsRequest {
  std::vector<OpcUaWsSubscriptionId> subscription_ids;
};

struct OpcUaWsDeleteSubscriptionsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct OpcUaWsCreateMonitoredItemsRequest {
  OpcUaWsSubscriptionId subscription_id = 0;
  OpcUaWsTimestampsToReturn timestamps_to_return =
      OpcUaWsTimestampsToReturn::Both;
  std::vector<OpcUaWsMonitoredItemCreateRequest> items_to_create;
};

struct OpcUaWsCreateMonitoredItemsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<OpcUaWsMonitoredItemCreateResult> results;
};

struct OpcUaWsModifyMonitoredItemsRequest {
  OpcUaWsSubscriptionId subscription_id = 0;
  OpcUaWsTimestampsToReturn timestamps_to_return =
      OpcUaWsTimestampsToReturn::Both;
  std::vector<OpcUaWsMonitoredItemModifyRequest> items_to_modify;
};

struct OpcUaWsModifyMonitoredItemsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<OpcUaWsMonitoredItemModifyResult> results;
};

struct OpcUaWsDeleteMonitoredItemsRequest {
  OpcUaWsSubscriptionId subscription_id = 0;
  std::vector<OpcUaWsMonitoredItemId> monitored_item_ids;
};

struct OpcUaWsDeleteMonitoredItemsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct OpcUaWsSetMonitoringModeRequest {
  OpcUaWsSubscriptionId subscription_id = 0;
  OpcUaWsMonitoringMode monitoring_mode = OpcUaWsMonitoringMode::Reporting;
  std::vector<OpcUaWsMonitoredItemId> monitored_item_ids;
};

struct OpcUaWsSetMonitoringModeResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

using OpcUaWsRequestBody =
    std::variant<OpcUaWsCreateSessionRequest,
                 OpcUaWsActivateSessionRequest,
                 OpcUaWsCloseSessionRequest,
                 OpcUaWsCreateSubscriptionRequest,
                 OpcUaWsModifySubscriptionRequest,
                 OpcUaWsSetPublishingModeRequest,
                 OpcUaWsDeleteSubscriptionsRequest,
                 OpcUaWsCreateMonitoredItemsRequest,
                 OpcUaWsModifyMonitoredItemsRequest,
                 OpcUaWsDeleteMonitoredItemsRequest,
                 OpcUaWsSetMonitoringModeRequest,
                 ReadRequest,
                 WriteRequest,
                 BrowseRequest,
                 TranslateBrowsePathsRequest,
                 CallRequest,
                 HistoryReadRawRequest,
                 HistoryReadEventsRequest,
                 AddNodesRequest,
                 DeleteNodesRequest,
                 AddReferencesRequest,
                 DeleteReferencesRequest>;

using OpcUaWsResponseBody =
    std::variant<OpcUaWsCreateSessionResponse,
                 OpcUaWsActivateSessionResponse,
                 OpcUaWsCloseSessionResponse,
                 OpcUaWsCreateSubscriptionResponse,
                 OpcUaWsModifySubscriptionResponse,
                 OpcUaWsSetPublishingModeResponse,
                 OpcUaWsDeleteSubscriptionsResponse,
                 OpcUaWsCreateMonitoredItemsResponse,
                 OpcUaWsModifyMonitoredItemsResponse,
                 OpcUaWsDeleteMonitoredItemsResponse,
                 OpcUaWsSetMonitoringModeResponse,
                 OpcUaWsServiceFault,
                 ReadResponse,
                 WriteResponse,
                 BrowseResponse,
                 TranslateBrowsePathsResponse,
                 CallResponse,
                 HistoryReadRawResponse,
                 HistoryReadEventsResponse,
                 AddNodesResponse,
                 DeleteNodesResponse,
                 AddReferencesResponse,
                 DeleteReferencesResponse>;

struct OpcUaWsRequestMessage {
  scada::UInt32 request_handle = 0;
  OpcUaWsRequestBody body;
};

struct OpcUaWsResponseMessage {
  scada::UInt32 request_handle = 0;
  OpcUaWsResponseBody body;
};

}  // namespace opcua_ws
