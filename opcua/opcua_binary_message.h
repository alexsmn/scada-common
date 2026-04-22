#pragma once

#include "opcua/opcua_message.h"

namespace opcua {

using OpcUaBinaryServiceFault = OpcUaServiceFault;
using OpcUaBinaryCreateSessionRequest = OpcUaCreateSessionRequest;
using OpcUaBinaryCreateSessionResponse = OpcUaCreateSessionResponse;
using OpcUaBinaryActivateSessionRequest = OpcUaActivateSessionRequest;
using OpcUaBinaryActivateSessionResponse = OpcUaActivateSessionResponse;
using OpcUaBinaryCloseSessionRequest = OpcUaCloseSessionRequest;
using OpcUaBinaryCloseSessionResponse = OpcUaCloseSessionResponse;

using OpcUaBinarySubscriptionId = OpcUaSubscriptionId;
using OpcUaBinaryMonitoredItemId = OpcUaMonitoredItemId;
using OpcUaBinaryMonitoringMode = OpcUaMonitoringMode;
using OpcUaBinaryTimestampsToReturn = OpcUaTimestampsToReturn;
using OpcUaBinaryDeadbandType = OpcUaDeadbandType;
using OpcUaBinaryDataChangeTrigger = OpcUaDataChangeTrigger;
using OpcUaBinaryDataChangeFilter = OpcUaDataChangeFilter;
using OpcUaBinaryMonitoringFilter = OpcUaMonitoringFilter;
using OpcUaBinaryMonitoringParameters = OpcUaMonitoringParameters;
using OpcUaBinaryMonitoredItemCreateRequest = OpcUaMonitoredItemCreateRequest;
using OpcUaBinaryMonitoredItemCreateResult = OpcUaMonitoredItemCreateResult;
using OpcUaBinaryMonitoredItemModifyRequest = OpcUaMonitoredItemModifyRequest;
using OpcUaBinaryMonitoredItemModifyResult = OpcUaMonitoredItemModifyResult;
using OpcUaBinarySubscriptionParameters = OpcUaSubscriptionParameters;
using OpcUaBinaryCreateSubscriptionRequest = OpcUaCreateSubscriptionRequest;
using OpcUaBinaryCreateSubscriptionResponse = OpcUaCreateSubscriptionResponse;
using OpcUaBinaryModifySubscriptionRequest = OpcUaModifySubscriptionRequest;
using OpcUaBinaryModifySubscriptionResponse = OpcUaModifySubscriptionResponse;
using OpcUaBinarySetPublishingModeRequest = OpcUaSetPublishingModeRequest;
using OpcUaBinarySetPublishingModeResponse = OpcUaSetPublishingModeResponse;
using OpcUaBinaryDeleteSubscriptionsRequest = OpcUaDeleteSubscriptionsRequest;
using OpcUaBinaryDeleteSubscriptionsResponse = OpcUaDeleteSubscriptionsResponse;
using OpcUaBinaryCreateMonitoredItemsRequest = OpcUaCreateMonitoredItemsRequest;
using OpcUaBinaryCreateMonitoredItemsResponse = OpcUaCreateMonitoredItemsResponse;
using OpcUaBinaryModifyMonitoredItemsRequest = OpcUaModifyMonitoredItemsRequest;
using OpcUaBinaryModifyMonitoredItemsResponse = OpcUaModifyMonitoredItemsResponse;
using OpcUaBinaryDeleteMonitoredItemsRequest = OpcUaDeleteMonitoredItemsRequest;
using OpcUaBinaryDeleteMonitoredItemsResponse = OpcUaDeleteMonitoredItemsResponse;
using OpcUaBinarySetMonitoringModeRequest = OpcUaSetMonitoringModeRequest;
using OpcUaBinarySetMonitoringModeResponse = OpcUaSetMonitoringModeResponse;
using OpcUaBinarySubscriptionAcknowledgement = OpcUaSubscriptionAcknowledgement;
using OpcUaBinaryMonitoredItemNotification = OpcUaMonitoredItemNotification;
using OpcUaBinaryEventFieldList = OpcUaEventFieldList;
using OpcUaBinaryDataChangeNotification = OpcUaDataChangeNotification;
using OpcUaBinaryEventNotificationList = OpcUaEventNotificationList;
using OpcUaBinaryStatusChangeNotification = OpcUaStatusChangeNotification;
using OpcUaBinaryNotificationData = OpcUaNotificationData;
using OpcUaBinaryNotificationMessage = OpcUaNotificationMessage;
using OpcUaBinaryPublishRequest = OpcUaPublishRequest;
using OpcUaBinaryPublishResponse = OpcUaPublishResponse;
using OpcUaBinaryRepublishRequest = OpcUaRepublishRequest;
using OpcUaBinaryRepublishResponse = OpcUaRepublishResponse;
using OpcUaBinaryTransferSubscriptionsRequest = OpcUaTransferSubscriptionsRequest;
using OpcUaBinaryTransferSubscriptionsResponse = OpcUaTransferSubscriptionsResponse;

using OpcUaBinaryReadRequest = ReadRequest;
using OpcUaBinaryReadResponse = ReadResponse;
using OpcUaBinaryWriteRequest = WriteRequest;
using OpcUaBinaryWriteResponse = WriteResponse;
using OpcUaBinaryBrowseRequest = BrowseRequest;
using OpcUaBinaryBrowseResponse = BrowseResponse;
using OpcUaBinaryBrowseNextRequest = BrowseNextRequest;
using OpcUaBinaryBrowseNextResponse = BrowseNextResponse;
using OpcUaBinaryTranslateBrowsePathsRequest = TranslateBrowsePathsRequest;
using OpcUaBinaryTranslateBrowsePathsResponse = TranslateBrowsePathsResponse;
using OpcUaBinaryCallMethodRequest = OpcUaMethodCallRequest;
using OpcUaBinaryCallMethodResult = OpcUaMethodCallResult;
using OpcUaBinaryCallRequest = CallRequest;
using OpcUaBinaryCallResponse = CallResponse;
using OpcUaBinaryHistoryReadRawRequest = HistoryReadRawRequest;
using OpcUaBinaryHistoryReadRawResponse = HistoryReadRawResponse;
using OpcUaBinaryHistoryReadEventsRequest = HistoryReadEventsRequest;
using OpcUaBinaryHistoryReadEventsResponse = HistoryReadEventsResponse;
using OpcUaBinaryAddNodesRequest = AddNodesRequest;
using OpcUaBinaryAddNodesResponse = AddNodesResponse;
using OpcUaBinaryDeleteNodesRequest = DeleteNodesRequest;
using OpcUaBinaryDeleteNodesResponse = DeleteNodesResponse;
using OpcUaBinaryAddReferencesRequest = AddReferencesRequest;
using OpcUaBinaryAddReferencesResponse = AddReferencesResponse;
using OpcUaBinaryDeleteReferencesRequest = DeleteReferencesRequest;
using OpcUaBinaryDeleteReferencesResponse = DeleteReferencesResponse;

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
