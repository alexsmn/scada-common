#include "opcua/binary/opcua_binary_message.h"

#include <gtest/gtest.h>

namespace opcua {
namespace {

TEST(OpcUaBinaryMessageTest,
     CanonicalSemanticTypesRemainAvailableThroughBinaryAliasLayer) {
  static_assert(std::is_same_v<OpcUaBinaryServiceFault, OpcUaServiceFault>);
  static_assert(
      std::is_same_v<OpcUaBinaryCreateSessionRequest, OpcUaCreateSessionRequest>);
  static_assert(std::is_same_v<OpcUaBinaryCreateSessionResponse,
                               OpcUaCreateSessionResponse>);
  static_assert(std::is_same_v<OpcUaBinaryActivateSessionRequest,
                               OpcUaActivateSessionRequest>);
  static_assert(std::is_same_v<OpcUaBinaryActivateSessionResponse,
                               OpcUaActivateSessionResponse>);
  static_assert(
      std::is_same_v<OpcUaBinaryCloseSessionRequest, OpcUaCloseSessionRequest>);
  static_assert(std::is_same_v<OpcUaBinaryCloseSessionResponse,
                               OpcUaCloseSessionResponse>);
  static_assert(std::is_same_v<OpcUaBinaryCreateSubscriptionRequest,
                               OpcUaCreateSubscriptionRequest>);
  static_assert(std::is_same_v<OpcUaBinaryCreateSubscriptionResponse,
                               OpcUaCreateSubscriptionResponse>);
  static_assert(std::is_same_v<OpcUaBinaryCreateMonitoredItemsRequest,
                               OpcUaCreateMonitoredItemsRequest>);
  static_assert(std::is_same_v<OpcUaBinaryCreateMonitoredItemsResponse,
                               OpcUaCreateMonitoredItemsResponse>);
  static_assert(std::is_same_v<OpcUaBinaryPublishRequest, OpcUaPublishRequest>);
  static_assert(
      std::is_same_v<OpcUaBinaryPublishResponse, OpcUaPublishResponse>);
  static_assert(std::is_same_v<OpcUaBinaryReadRequest, ReadRequest>);
  static_assert(std::is_same_v<OpcUaBinaryReadResponse, ReadResponse>);
  static_assert(std::is_same_v<OpcUaBinaryWriteRequest, WriteRequest>);
  static_assert(std::is_same_v<OpcUaBinaryWriteResponse, WriteResponse>);
  static_assert(std::is_same_v<OpcUaBinaryCallRequest, CallRequest>);
  static_assert(std::is_same_v<OpcUaBinaryCallResponse, CallResponse>);
  static_assert(std::is_same_v<OpcUaBinaryHistoryReadRawRequest,
                               HistoryReadRawRequest>);
  static_assert(std::is_same_v<OpcUaBinaryHistoryReadRawResponse,
                               HistoryReadRawResponse>);
  static_assert(std::is_same_v<OpcUaBinaryHistoryReadEventsRequest,
                               HistoryReadEventsRequest>);
  static_assert(std::is_same_v<OpcUaBinaryHistoryReadEventsResponse,
                               HistoryReadEventsResponse>);
  static_assert(std::is_same_v<OpcUaBinaryAddNodesRequest, AddNodesRequest>);
  static_assert(std::is_same_v<OpcUaBinaryAddNodesResponse, AddNodesResponse>);
  static_assert(std::is_same_v<OpcUaBinaryDeleteNodesRequest,
                               DeleteNodesRequest>);
  static_assert(std::is_same_v<OpcUaBinaryDeleteNodesResponse,
                               DeleteNodesResponse>);
  static_assert(std::is_same_v<OpcUaBinaryRequestBody, OpcUaRequestBody>);
  static_assert(std::is_same_v<OpcUaBinaryResponseBody, OpcUaResponseBody>);

  const OpcUaBinaryRequestBody request =
      ReadRequest{.inputs = {{.node_id = {17u, 2},
                              .attribute_id = scada::AttributeId::Value}}};
  const auto* read = std::get_if<ReadRequest>(&request);
  ASSERT_NE(read, nullptr);
  ASSERT_EQ(read->inputs.size(), 1u);
  EXPECT_EQ(read->inputs[0].node_id, (scada::NodeId{17u, 2}));

  const OpcUaBinaryResponseBody response =
      OpcUaServiceFault{.status = scada::StatusCode::Bad_SessionIsLoggedOff};
  const auto* fault = std::get_if<OpcUaServiceFault>(&response);
  ASSERT_NE(fault, nullptr);
  EXPECT_EQ(fault->status.code(), scada::StatusCode::Bad_SessionIsLoggedOff);
}

}  // namespace
}  // namespace opcua
