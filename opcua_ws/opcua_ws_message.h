#pragma once

#include "opcua/opcua_message.h"

namespace opcua_ws {

using opcua::OpcUaCreateMonitoredItemsRequest;
using opcua::OpcUaCreateMonitoredItemsResponse;
using opcua::OpcUaCreateSubscriptionRequest;
using opcua::OpcUaCreateSubscriptionResponse;
using opcua::OpcUaDataChangeFilter;
using opcua::OpcUaDataChangeNotification;
using opcua::OpcUaDataChangeTrigger;
using opcua::OpcUaDeadbandType;
using opcua::OpcUaDeleteMonitoredItemsRequest;
using opcua::OpcUaDeleteMonitoredItemsResponse;
using opcua::OpcUaDeleteSubscriptionsRequest;
using opcua::OpcUaDeleteSubscriptionsResponse;
using opcua::OpcUaEventFieldList;
using opcua::OpcUaEventNotificationList;
using opcua::OpcUaModifyMonitoredItemsRequest;
using opcua::OpcUaModifyMonitoredItemsResponse;
using opcua::OpcUaModifySubscriptionRequest;
using opcua::OpcUaModifySubscriptionResponse;
using opcua::OpcUaMonitoredItemCreateRequest;
using opcua::OpcUaMonitoredItemCreateResult;
using opcua::OpcUaMonitoredItemId;
using opcua::OpcUaMonitoredItemModifyRequest;
using opcua::OpcUaMonitoredItemModifyResult;
using opcua::OpcUaMonitoredItemNotification;
using opcua::OpcUaMonitoringFilter;
using opcua::OpcUaMonitoringMode;
using opcua::OpcUaMonitoringParameters;
using opcua::OpcUaNotificationData;
using opcua::OpcUaNotificationMessage;
using opcua::OpcUaPublishRequest;
using opcua::OpcUaPublishResponse;
using opcua::OpcUaRepublishRequest;
using opcua::OpcUaRepublishResponse;
using opcua::OpcUaRequestBody;
using opcua::OpcUaRequestMessage;
using opcua::OpcUaResponseBody;
using opcua::OpcUaResponseMessage;
using opcua::OpcUaServiceFault;
using opcua::OpcUaSetMonitoringModeRequest;
using opcua::OpcUaSetMonitoringModeResponse;
using opcua::OpcUaSetPublishingModeRequest;
using opcua::OpcUaSetPublishingModeResponse;
using opcua::OpcUaStatusChangeNotification;
using opcua::OpcUaSubscriptionAcknowledgement;
using opcua::OpcUaSubscriptionId;
using opcua::OpcUaSubscriptionParameters;
using opcua::OpcUaTimestampsToReturn;
using opcua::OpcUaTransferSubscriptionsRequest;
using opcua::OpcUaTransferSubscriptionsResponse;

using OpcUaWsServiceFault = opcua::OpcUaServiceFault;
using OpcUaWsSubscriptionId = opcua::OpcUaSubscriptionId;
using OpcUaWsMonitoredItemId = opcua::OpcUaMonitoredItemId;
using OpcUaWsMonitoringMode = opcua::OpcUaMonitoringMode;
using OpcUaWsTimestampsToReturn = opcua::OpcUaTimestampsToReturn;
using OpcUaWsDeadbandType = opcua::OpcUaDeadbandType;
using OpcUaWsDataChangeTrigger = opcua::OpcUaDataChangeTrigger;
using OpcUaWsDataChangeFilter = opcua::OpcUaDataChangeFilter;
using OpcUaWsMonitoringFilter = opcua::OpcUaMonitoringFilter;
using OpcUaWsMonitoringParameters = opcua::OpcUaMonitoringParameters;
using OpcUaWsMonitoredItemCreateRequest =
    opcua::OpcUaMonitoredItemCreateRequest;
using OpcUaWsMonitoredItemCreateResult = opcua::OpcUaMonitoredItemCreateResult;
using OpcUaWsMonitoredItemModifyRequest =
    opcua::OpcUaMonitoredItemModifyRequest;
using OpcUaWsMonitoredItemModifyResult = opcua::OpcUaMonitoredItemModifyResult;
using OpcUaWsSubscriptionParameters = opcua::OpcUaSubscriptionParameters;
using OpcUaWsCreateSubscriptionRequest = opcua::OpcUaCreateSubscriptionRequest;
using OpcUaWsCreateSubscriptionResponse =
    opcua::OpcUaCreateSubscriptionResponse;
using OpcUaWsModifySubscriptionRequest = opcua::OpcUaModifySubscriptionRequest;
using OpcUaWsModifySubscriptionResponse =
    opcua::OpcUaModifySubscriptionResponse;
using OpcUaWsSetPublishingModeRequest = opcua::OpcUaSetPublishingModeRequest;
using OpcUaWsSetPublishingModeResponse = opcua::OpcUaSetPublishingModeResponse;
using OpcUaWsDeleteSubscriptionsRequest =
    opcua::OpcUaDeleteSubscriptionsRequest;
using OpcUaWsDeleteSubscriptionsResponse =
    opcua::OpcUaDeleteSubscriptionsResponse;
using OpcUaWsCreateMonitoredItemsRequest =
    opcua::OpcUaCreateMonitoredItemsRequest;
using OpcUaWsCreateMonitoredItemsResponse =
    opcua::OpcUaCreateMonitoredItemsResponse;
using OpcUaWsModifyMonitoredItemsRequest =
    opcua::OpcUaModifyMonitoredItemsRequest;
using OpcUaWsModifyMonitoredItemsResponse =
    opcua::OpcUaModifyMonitoredItemsResponse;
using OpcUaWsDeleteMonitoredItemsRequest =
    opcua::OpcUaDeleteMonitoredItemsRequest;
using OpcUaWsDeleteMonitoredItemsResponse =
    opcua::OpcUaDeleteMonitoredItemsResponse;
using OpcUaWsSetMonitoringModeRequest = opcua::OpcUaSetMonitoringModeRequest;
using OpcUaWsSetMonitoringModeResponse = opcua::OpcUaSetMonitoringModeResponse;
using OpcUaWsSubscriptionAcknowledgement =
    opcua::OpcUaSubscriptionAcknowledgement;
using OpcUaWsMonitoredItemNotification =
    opcua::OpcUaMonitoredItemNotification;
using OpcUaWsEventFieldList = opcua::OpcUaEventFieldList;
using OpcUaWsDataChangeNotification = opcua::OpcUaDataChangeNotification;
using OpcUaWsEventNotificationList = opcua::OpcUaEventNotificationList;
using OpcUaWsStatusChangeNotification = opcua::OpcUaStatusChangeNotification;
using OpcUaWsNotificationData = opcua::OpcUaNotificationData;
using OpcUaWsNotificationMessage = opcua::OpcUaNotificationMessage;
using OpcUaWsPublishRequest = opcua::OpcUaPublishRequest;
using OpcUaWsPublishResponse = opcua::OpcUaPublishResponse;
using OpcUaWsRepublishRequest = opcua::OpcUaRepublishRequest;
using OpcUaWsRepublishResponse = opcua::OpcUaRepublishResponse;
using OpcUaWsTransferSubscriptionsRequest =
    opcua::OpcUaTransferSubscriptionsRequest;
using OpcUaWsTransferSubscriptionsResponse =
    opcua::OpcUaTransferSubscriptionsResponse;
using OpcUaWsRequestBody = opcua::OpcUaRequestBody;
using OpcUaWsResponseBody = opcua::OpcUaResponseBody;
using OpcUaWsRequestMessage = opcua::OpcUaRequestMessage;
using OpcUaWsResponseMessage = opcua::OpcUaResponseMessage;

}  // namespace opcua_ws
