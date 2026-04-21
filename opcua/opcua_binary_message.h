#pragma once

#include "opcua_ws/opcua_ws_message.h"

namespace opcua {

using OpcUaBinaryServiceFault = opcua_ws::OpcUaWsServiceFault;
using OpcUaBinaryCreateSessionRequest = opcua_ws::OpcUaWsCreateSessionRequest;
using OpcUaBinaryCreateSessionResponse =
    opcua_ws::OpcUaWsCreateSessionResponse;
using OpcUaBinaryActivateSessionRequest =
    opcua_ws::OpcUaWsActivateSessionRequest;
using OpcUaBinaryActivateSessionResponse =
    opcua_ws::OpcUaWsActivateSessionResponse;
using OpcUaBinaryCloseSessionRequest = opcua_ws::OpcUaWsCloseSessionRequest;
using OpcUaBinaryCloseSessionResponse = opcua_ws::OpcUaWsCloseSessionResponse;

using OpcUaBinarySubscriptionId = opcua_ws::OpcUaWsSubscriptionId;
using OpcUaBinaryMonitoredItemId = opcua_ws::OpcUaWsMonitoredItemId;
using OpcUaBinaryMonitoringMode = opcua_ws::OpcUaWsMonitoringMode;
using OpcUaBinaryTimestampsToReturn = opcua_ws::OpcUaWsTimestampsToReturn;
using OpcUaBinaryDeadbandType = opcua_ws::OpcUaWsDeadbandType;
using OpcUaBinaryDataChangeTrigger = opcua_ws::OpcUaWsDataChangeTrigger;
using OpcUaBinaryDataChangeFilter = opcua_ws::OpcUaWsDataChangeFilter;
using OpcUaBinaryMonitoringFilter = opcua_ws::OpcUaWsMonitoringFilter;
using OpcUaBinaryMonitoringParameters = opcua_ws::OpcUaWsMonitoringParameters;
using OpcUaBinaryMonitoredItemCreateRequest =
    opcua_ws::OpcUaWsMonitoredItemCreateRequest;
using OpcUaBinaryMonitoredItemCreateResult =
    opcua_ws::OpcUaWsMonitoredItemCreateResult;
using OpcUaBinaryMonitoredItemModifyRequest =
    opcua_ws::OpcUaWsMonitoredItemModifyRequest;
using OpcUaBinaryMonitoredItemModifyResult =
    opcua_ws::OpcUaWsMonitoredItemModifyResult;
using OpcUaBinarySubscriptionParameters =
    opcua_ws::OpcUaWsSubscriptionParameters;
using OpcUaBinaryCreateSubscriptionRequest =
    opcua_ws::OpcUaWsCreateSubscriptionRequest;
using OpcUaBinaryCreateSubscriptionResponse =
    opcua_ws::OpcUaWsCreateSubscriptionResponse;
using OpcUaBinaryModifySubscriptionRequest =
    opcua_ws::OpcUaWsModifySubscriptionRequest;
using OpcUaBinaryModifySubscriptionResponse =
    opcua_ws::OpcUaWsModifySubscriptionResponse;
using OpcUaBinarySetPublishingModeRequest =
    opcua_ws::OpcUaWsSetPublishingModeRequest;
using OpcUaBinarySetPublishingModeResponse =
    opcua_ws::OpcUaWsSetPublishingModeResponse;
using OpcUaBinaryDeleteSubscriptionsRequest =
    opcua_ws::OpcUaWsDeleteSubscriptionsRequest;
using OpcUaBinaryDeleteSubscriptionsResponse =
    opcua_ws::OpcUaWsDeleteSubscriptionsResponse;
using OpcUaBinaryCreateMonitoredItemsRequest =
    opcua_ws::OpcUaWsCreateMonitoredItemsRequest;
using OpcUaBinaryCreateMonitoredItemsResponse =
    opcua_ws::OpcUaWsCreateMonitoredItemsResponse;
using OpcUaBinaryModifyMonitoredItemsRequest =
    opcua_ws::OpcUaWsModifyMonitoredItemsRequest;
using OpcUaBinaryModifyMonitoredItemsResponse =
    opcua_ws::OpcUaWsModifyMonitoredItemsResponse;
using OpcUaBinaryDeleteMonitoredItemsRequest =
    opcua_ws::OpcUaWsDeleteMonitoredItemsRequest;
using OpcUaBinaryDeleteMonitoredItemsResponse =
    opcua_ws::OpcUaWsDeleteMonitoredItemsResponse;
using OpcUaBinarySetMonitoringModeRequest =
    opcua_ws::OpcUaWsSetMonitoringModeRequest;
using OpcUaBinarySetMonitoringModeResponse =
    opcua_ws::OpcUaWsSetMonitoringModeResponse;
using OpcUaBinarySubscriptionAcknowledgement =
    opcua_ws::OpcUaWsSubscriptionAcknowledgement;
using OpcUaBinaryMonitoredItemNotification =
    opcua_ws::OpcUaWsMonitoredItemNotification;
using OpcUaBinaryEventFieldList = opcua_ws::OpcUaWsEventFieldList;
using OpcUaBinaryDataChangeNotification =
    opcua_ws::OpcUaWsDataChangeNotification;
using OpcUaBinaryEventNotificationList =
    opcua_ws::OpcUaWsEventNotificationList;
using OpcUaBinaryStatusChangeNotification =
    opcua_ws::OpcUaWsStatusChangeNotification;
using OpcUaBinaryNotificationData = opcua_ws::OpcUaWsNotificationData;
using OpcUaBinaryNotificationMessage = opcua_ws::OpcUaWsNotificationMessage;
using OpcUaBinaryPublishRequest = opcua_ws::OpcUaWsPublishRequest;
using OpcUaBinaryPublishResponse = opcua_ws::OpcUaWsPublishResponse;
using OpcUaBinaryRepublishRequest = opcua_ws::OpcUaWsRepublishRequest;
using OpcUaBinaryRepublishResponse = opcua_ws::OpcUaWsRepublishResponse;
using OpcUaBinaryTransferSubscriptionsRequest =
    opcua_ws::OpcUaWsTransferSubscriptionsRequest;
using OpcUaBinaryTransferSubscriptionsResponse =
    opcua_ws::OpcUaWsTransferSubscriptionsResponse;

using OpcUaBinaryReadRequest = opcua_ws::ReadRequest;
using OpcUaBinaryReadResponse = opcua_ws::ReadResponse;
using OpcUaBinaryWriteRequest = opcua_ws::WriteRequest;
using OpcUaBinaryWriteResponse = opcua_ws::WriteResponse;
using OpcUaBinaryBrowseRequest = opcua_ws::BrowseRequest;
using OpcUaBinaryBrowseResponse = opcua_ws::BrowseResponse;
using OpcUaBinaryBrowseNextRequest = opcua_ws::BrowseNextRequest;
using OpcUaBinaryBrowseNextResponse = opcua_ws::BrowseNextResponse;
using OpcUaBinaryTranslateBrowsePathsRequest =
    opcua_ws::TranslateBrowsePathsRequest;
using OpcUaBinaryTranslateBrowsePathsResponse =
    opcua_ws::TranslateBrowsePathsResponse;
using OpcUaBinaryCallMethodRequest = opcua_ws::CallMethodRequest;
using OpcUaBinaryCallMethodResult = opcua_ws::CallMethodResult;
using OpcUaBinaryCallRequest = opcua_ws::CallRequest;
using OpcUaBinaryCallResponse = opcua_ws::CallResponse;
using OpcUaBinaryHistoryReadRawRequest = opcua_ws::HistoryReadRawRequest;
using OpcUaBinaryHistoryReadRawResponse = opcua_ws::HistoryReadRawResponse;
using OpcUaBinaryHistoryReadEventsRequest =
    opcua_ws::HistoryReadEventsRequest;
using OpcUaBinaryHistoryReadEventsResponse =
    opcua_ws::HistoryReadEventsResponse;
using OpcUaBinaryAddNodesRequest = opcua_ws::AddNodesRequest;
using OpcUaBinaryAddNodesResponse = opcua_ws::AddNodesResponse;
using OpcUaBinaryDeleteNodesRequest = opcua_ws::DeleteNodesRequest;
using OpcUaBinaryDeleteNodesResponse = opcua_ws::DeleteNodesResponse;
using OpcUaBinaryAddReferencesRequest = opcua_ws::AddReferencesRequest;
using OpcUaBinaryAddReferencesResponse = opcua_ws::AddReferencesResponse;
using OpcUaBinaryDeleteReferencesRequest =
    opcua_ws::DeleteReferencesRequest;
using OpcUaBinaryDeleteReferencesResponse =
    opcua_ws::DeleteReferencesResponse;

using OpcUaBinaryRequestBody =
    std::variant<OpcUaBinaryCreateSessionRequest,
                 OpcUaBinaryActivateSessionRequest,
                 OpcUaBinaryCloseSessionRequest,
                 OpcUaBinaryCreateSubscriptionRequest,
                 OpcUaBinaryModifySubscriptionRequest,
                 OpcUaBinarySetPublishingModeRequest,
                 OpcUaBinaryDeleteSubscriptionsRequest,
                 OpcUaBinaryPublishRequest,
                 OpcUaBinaryRepublishRequest,
                 OpcUaBinaryTransferSubscriptionsRequest,
                 OpcUaBinaryCreateMonitoredItemsRequest,
                 OpcUaBinaryModifyMonitoredItemsRequest,
                 OpcUaBinaryDeleteMonitoredItemsRequest,
                 OpcUaBinarySetMonitoringModeRequest,
                 OpcUaBinaryReadRequest,
                 OpcUaBinaryWriteRequest,
                 OpcUaBinaryBrowseRequest,
                 OpcUaBinaryBrowseNextRequest,
                 OpcUaBinaryTranslateBrowsePathsRequest,
                 OpcUaBinaryCallRequest,
                 OpcUaBinaryHistoryReadRawRequest,
                 OpcUaBinaryHistoryReadEventsRequest,
                 OpcUaBinaryAddNodesRequest,
                 OpcUaBinaryDeleteNodesRequest,
                 OpcUaBinaryAddReferencesRequest,
                 OpcUaBinaryDeleteReferencesRequest>;

using OpcUaBinaryResponseBody =
    std::variant<OpcUaBinaryCreateSessionResponse,
                 OpcUaBinaryActivateSessionResponse,
                 OpcUaBinaryCloseSessionResponse,
                 OpcUaBinaryCreateSubscriptionResponse,
                 OpcUaBinaryModifySubscriptionResponse,
                 OpcUaBinarySetPublishingModeResponse,
                 OpcUaBinaryDeleteSubscriptionsResponse,
                 OpcUaBinaryPublishResponse,
                 OpcUaBinaryRepublishResponse,
                 OpcUaBinaryTransferSubscriptionsResponse,
                 OpcUaBinaryCreateMonitoredItemsResponse,
                 OpcUaBinaryModifyMonitoredItemsResponse,
                 OpcUaBinaryDeleteMonitoredItemsResponse,
                 OpcUaBinarySetMonitoringModeResponse,
                 OpcUaBinaryServiceFault,
                 OpcUaBinaryReadResponse,
                 OpcUaBinaryWriteResponse,
                 OpcUaBinaryBrowseResponse,
                 OpcUaBinaryBrowseNextResponse,
                 OpcUaBinaryTranslateBrowsePathsResponse,
                 OpcUaBinaryCallResponse,
                 OpcUaBinaryHistoryReadRawResponse,
                 OpcUaBinaryHistoryReadEventsResponse,
                 OpcUaBinaryAddNodesResponse,
                 OpcUaBinaryDeleteNodesResponse,
                 OpcUaBinaryAddReferencesResponse,
                 OpcUaBinaryDeleteReferencesResponse>;

}  // namespace opcua
