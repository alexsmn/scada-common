#pragma once

#include "opcua/opcua_server_session_manager.h"
#include "opcua/opcua_service_message.h"

#include <boost/json/value.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace opcua {

struct OpcUaServiceFault {
  scada::Status status{scada::StatusCode::Bad};
};

using OpcUaSubscriptionId = scada::UInt32;
using OpcUaMonitoredItemId = scada::UInt32;

enum class OpcUaMonitoringMode : scada::UInt32 {
  Disabled = 0,
  Sampling = 1,
  Reporting = 2,
};

enum class OpcUaTimestampsToReturn : scada::UInt32 {
  Source = 0,
  Server = 1,
  Both = 2,
  Neither = 3,
};

enum class OpcUaDeadbandType : scada::UInt32 {
  None = 0,
  Absolute = 1,
  Percent = 2,
};

enum class OpcUaDataChangeTrigger : scada::UInt32 {
  Status = 0,
  StatusValue = 1,
  StatusValueTimestamp = 2,
};

struct OpcUaDataChangeFilter {
  bool operator==(const OpcUaDataChangeFilter&) const = default;

  OpcUaDataChangeTrigger trigger = OpcUaDataChangeTrigger::StatusValue;
  OpcUaDeadbandType deadband_type = OpcUaDeadbandType::None;
  double deadband_value = 0;
};

using OpcUaMonitoringFilter =
    std::variant<OpcUaDataChangeFilter, boost::json::value>;

struct OpcUaMonitoringParameters {
  bool operator==(const OpcUaMonitoringParameters&) const = default;

  scada::UInt32 client_handle = 0;
  double sampling_interval_ms = 0;
  std::optional<OpcUaMonitoringFilter> filter;
  scada::UInt32 queue_size = 1;
  bool discard_oldest = true;
};

struct OpcUaMonitoredItemCreateRequest {
  bool operator==(const OpcUaMonitoredItemCreateRequest&) const = default;

  scada::ReadValueId item_to_monitor;
  std::optional<std::string> index_range;
  OpcUaMonitoringMode monitoring_mode = OpcUaMonitoringMode::Reporting;
  OpcUaMonitoringParameters requested_parameters;
};

struct OpcUaMonitoredItemCreateResult {
  bool operator==(const OpcUaMonitoredItemCreateResult&) const = default;

  scada::Status status{scada::StatusCode::Good};
  OpcUaMonitoredItemId monitored_item_id = 0;
  double revised_sampling_interval_ms = 0;
  scada::UInt32 revised_queue_size = 0;
  std::optional<boost::json::value> filter_result;
};

struct OpcUaMonitoredItemModifyRequest {
  bool operator==(const OpcUaMonitoredItemModifyRequest&) const = default;

  OpcUaMonitoredItemId monitored_item_id = 0;
  OpcUaMonitoringParameters requested_parameters;
};

struct OpcUaMonitoredItemModifyResult {
  bool operator==(const OpcUaMonitoredItemModifyResult&) const = default;

  scada::Status status{scada::StatusCode::Good};
  double revised_sampling_interval_ms = 0;
  scada::UInt32 revised_queue_size = 0;
  std::optional<boost::json::value> filter_result;
};

struct OpcUaSubscriptionParameters {
  bool operator==(const OpcUaSubscriptionParameters&) const = default;

  double publishing_interval_ms = 0;
  scada::UInt32 lifetime_count = 0;
  scada::UInt32 max_keep_alive_count = 0;
  scada::UInt32 max_notifications_per_publish = 0;
  bool publishing_enabled = true;
  scada::UInt8 priority = 0;
};

struct OpcUaCreateSubscriptionRequest {
  OpcUaSubscriptionParameters parameters;
};

struct OpcUaCreateSubscriptionResponse {
  scada::Status status{scada::StatusCode::Good};
  OpcUaSubscriptionId subscription_id = 0;
  double revised_publishing_interval_ms = 0;
  scada::UInt32 revised_lifetime_count = 0;
  scada::UInt32 revised_max_keep_alive_count = 0;
};

struct OpcUaModifySubscriptionRequest {
  OpcUaSubscriptionId subscription_id = 0;
  OpcUaSubscriptionParameters parameters;
};

struct OpcUaModifySubscriptionResponse {
  scada::Status status{scada::StatusCode::Good};
  double revised_publishing_interval_ms = 0;
  scada::UInt32 revised_lifetime_count = 0;
  scada::UInt32 revised_max_keep_alive_count = 0;
};

struct OpcUaSetPublishingModeRequest {
  bool publishing_enabled = true;
  std::vector<OpcUaSubscriptionId> subscription_ids;
};

struct OpcUaSetPublishingModeResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct OpcUaDeleteSubscriptionsRequest {
  std::vector<OpcUaSubscriptionId> subscription_ids;
};

struct OpcUaDeleteSubscriptionsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct OpcUaCreateMonitoredItemsRequest {
  OpcUaSubscriptionId subscription_id = 0;
  OpcUaTimestampsToReturn timestamps_to_return =
      OpcUaTimestampsToReturn::Both;
  std::vector<OpcUaMonitoredItemCreateRequest> items_to_create;
};

struct OpcUaCreateMonitoredItemsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<OpcUaMonitoredItemCreateResult> results;
};

struct OpcUaModifyMonitoredItemsRequest {
  OpcUaSubscriptionId subscription_id = 0;
  OpcUaTimestampsToReturn timestamps_to_return =
      OpcUaTimestampsToReturn::Both;
  std::vector<OpcUaMonitoredItemModifyRequest> items_to_modify;
};

struct OpcUaModifyMonitoredItemsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<OpcUaMonitoredItemModifyResult> results;
};

struct OpcUaDeleteMonitoredItemsRequest {
  OpcUaSubscriptionId subscription_id = 0;
  std::vector<OpcUaMonitoredItemId> monitored_item_ids;
};

struct OpcUaDeleteMonitoredItemsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct OpcUaSetMonitoringModeRequest {
  OpcUaSubscriptionId subscription_id = 0;
  OpcUaMonitoringMode monitoring_mode = OpcUaMonitoringMode::Reporting;
  std::vector<OpcUaMonitoredItemId> monitored_item_ids;
};

struct OpcUaSetMonitoringModeResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

struct OpcUaSubscriptionAcknowledgement {
  bool operator==(const OpcUaSubscriptionAcknowledgement&) const = default;

  OpcUaSubscriptionId subscription_id = 0;
  scada::UInt32 sequence_number = 0;
};

struct OpcUaMonitoredItemNotification {
  bool operator==(const OpcUaMonitoredItemNotification&) const = default;

  scada::UInt32 client_handle = 0;
  scada::DataValue value;
};

struct OpcUaEventFieldList {
  bool operator==(const OpcUaEventFieldList&) const = default;

  scada::UInt32 client_handle = 0;
  std::vector<scada::Variant> event_fields;
};

struct OpcUaDataChangeNotification {
  bool operator==(const OpcUaDataChangeNotification&) const = default;

  std::vector<OpcUaMonitoredItemNotification> monitored_items;
};

struct OpcUaEventNotificationList {
  bool operator==(const OpcUaEventNotificationList&) const = default;

  std::vector<OpcUaEventFieldList> events;
};

struct OpcUaStatusChangeNotification {
  bool operator==(const OpcUaStatusChangeNotification&) const = default;

  scada::StatusCode status = scada::StatusCode::Good;
};

using OpcUaNotificationData =
    std::variant<OpcUaDataChangeNotification,
                 OpcUaEventNotificationList,
                 OpcUaStatusChangeNotification>;

struct OpcUaNotificationMessage {
  bool operator==(const OpcUaNotificationMessage&) const = default;

  scada::UInt32 sequence_number = 0;
  base::Time publish_time;
  std::vector<OpcUaNotificationData> notification_data;
};

struct OpcUaPublishRequest {
  std::vector<OpcUaSubscriptionAcknowledgement> subscription_acknowledgements;
};

struct OpcUaPublishResponse {
  scada::Status status{scada::StatusCode::Good};
  OpcUaSubscriptionId subscription_id = 0;
  std::vector<scada::StatusCode> results;
  bool more_notifications = false;
  OpcUaNotificationMessage notification_message;
  std::vector<OpcUaSubscriptionId> available_sequence_numbers;
};

struct OpcUaRepublishRequest {
  OpcUaSubscriptionId subscription_id = 0;
  scada::UInt32 retransmit_sequence_number = 0;
};

struct OpcUaRepublishResponse {
  scada::Status status{scada::StatusCode::Good};
  OpcUaNotificationMessage notification_message;
};

struct OpcUaTransferSubscriptionsRequest {
  std::vector<OpcUaSubscriptionId> subscription_ids;
  bool send_initial_values = false;
};

struct OpcUaTransferSubscriptionsResponse {
  scada::Status status{scada::StatusCode::Good};
  std::vector<scada::StatusCode> results;
};

using OpcUaRequestBody =
    std::variant<OpcUaCreateSessionRequest,
                 OpcUaActivateSessionRequest,
                 OpcUaCloseSessionRequest,
                 OpcUaCreateSubscriptionRequest,
                 OpcUaModifySubscriptionRequest,
                 OpcUaSetPublishingModeRequest,
                 OpcUaDeleteSubscriptionsRequest,
                 OpcUaPublishRequest,
                 OpcUaRepublishRequest,
                 OpcUaTransferSubscriptionsRequest,
                 OpcUaCreateMonitoredItemsRequest,
                 OpcUaModifyMonitoredItemsRequest,
                 OpcUaDeleteMonitoredItemsRequest,
                 OpcUaSetMonitoringModeRequest,
                 ReadRequest,
                 WriteRequest,
                 BrowseRequest,
                 BrowseNextRequest,
                 TranslateBrowsePathsRequest,
                 CallRequest,
                 HistoryReadRawRequest,
                 HistoryReadEventsRequest,
                 AddNodesRequest,
                 DeleteNodesRequest,
                 AddReferencesRequest,
                 DeleteReferencesRequest>;

using OpcUaResponseBody =
    std::variant<OpcUaCreateSessionResponse,
                 OpcUaActivateSessionResponse,
                 OpcUaCloseSessionResponse,
                 OpcUaCreateSubscriptionResponse,
                 OpcUaModifySubscriptionResponse,
                 OpcUaSetPublishingModeResponse,
                 OpcUaDeleteSubscriptionsResponse,
                 OpcUaPublishResponse,
                 OpcUaRepublishResponse,
                 OpcUaTransferSubscriptionsResponse,
                 OpcUaCreateMonitoredItemsResponse,
                 OpcUaModifyMonitoredItemsResponse,
                 OpcUaDeleteMonitoredItemsResponse,
                 OpcUaSetMonitoringModeResponse,
                 OpcUaServiceFault,
                 ReadResponse,
                 WriteResponse,
                 BrowseResponse,
                 BrowseNextResponse,
                 TranslateBrowsePathsResponse,
                 CallResponse,
                 HistoryReadRawResponse,
                 HistoryReadEventsResponse,
                 AddNodesResponse,
                 DeleteNodesResponse,
                 AddReferencesResponse,
                 DeleteReferencesResponse>;

struct OpcUaRequestMessage {
  scada::UInt32 request_handle = 0;
  OpcUaRequestBody body;
};

struct OpcUaResponseMessage {
  scada::UInt32 request_handle = 0;
  OpcUaResponseBody body;
};

}  // namespace opcua
