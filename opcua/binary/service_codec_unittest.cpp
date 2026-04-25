#include "opcua/binary/service_codec.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace opcua {
namespace {

// Every test below encodes a server-side response via the existing
// EncodeOpcUaBinaryServiceResponse, then feeds the bytes through the new
// client-side DecodeOpcUaBinaryServiceResponse and asserts the typed payload
// survives the round trip. This pins the inverse-pair contract without
// pulling in a live channel or transport.

template <typename Body>
OpcUaBinaryDecodedResponse RoundTrip(std::uint32_t request_handle, Body body) {
  const auto encoded = EncodeOpcUaBinaryServiceResponse(
      request_handle, OpcUaResponseBody{std::move(body)});
  EXPECT_TRUE(encoded.has_value());
  const auto decoded = DecodeOpcUaBinaryServiceResponse(*encoded);
  EXPECT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->request_handle, request_handle);
  return *decoded;
}

TEST(OpcUaBinaryServiceCodecTest, CreateSessionResponseRoundTrip) {
  OpcUaCreateSessionResponse response{
      .status = scada::StatusCode::Good,
      .session_id = scada::NodeId{42},
      .authentication_token = scada::NodeId{43},
      .server_nonce = scada::ByteString{'n', 'o', 'n', 'c', 'e'},
      .revised_timeout = base::TimeDelta::FromMilliseconds(60000),
  };
  const auto decoded = RoundTrip(17, response);
  const auto& typed = std::get<OpcUaCreateSessionResponse>(decoded.body);
  EXPECT_TRUE(typed.status.good());
  EXPECT_EQ(typed.session_id, response.session_id);
  EXPECT_EQ(typed.authentication_token, response.authentication_token);
  EXPECT_EQ(typed.revised_timeout.InMilliseconds(), 60000);
  EXPECT_EQ(typed.server_nonce, response.server_nonce);
}

TEST(OpcUaBinaryServiceCodecTest, ActivateSessionResponseRoundTrip) {
  OpcUaActivateSessionResponse response{.status = scada::StatusCode::Good};
  const auto decoded = RoundTrip(5, response);
  const auto& typed = std::get<OpcUaActivateSessionResponse>(decoded.body);
  EXPECT_TRUE(typed.status.good());
}

TEST(OpcUaBinaryServiceCodecTest, CloseSessionResponseRoundTrip) {
  OpcUaCloseSessionResponse response{.status = scada::StatusCode::Good};
  const auto decoded = RoundTrip(9, response);
  const auto& typed = std::get<OpcUaCloseSessionResponse>(decoded.body);
  EXPECT_TRUE(typed.status.good());
}

TEST(OpcUaBinaryServiceCodecTest, ReadResponseRoundTrip) {
  scada::DataValue bad_value{scada::Variant{std::int32_t{-1}}, {}, {}, {}};
  bad_value.status_code = scada::StatusCode::Bad;
  ReadResponse response{
      .status = scada::StatusCode::Good,
      .results = {scada::DataValue{scada::Variant{std::int32_t{42}}, {}, {}, {}},
                  bad_value},
  };
  const auto decoded = RoundTrip(11, response);
  const auto& typed = std::get<ReadResponse>(decoded.body);
  EXPECT_TRUE(typed.status.good());
  ASSERT_EQ(typed.results.size(), 2u);
  EXPECT_EQ(typed.results[0].value, response.results[0].value);
  EXPECT_TRUE(scada::IsGood(typed.results[0].status_code));
  EXPECT_EQ(typed.results[1].value, response.results[1].value);
  EXPECT_EQ(typed.results[1].status_code, scada::StatusCode::Bad);
}

TEST(OpcUaBinaryServiceCodecTest, WriteResponseRoundTrip) {
  WriteResponse response{
      .status = scada::StatusCode::Good,
      .results = {scada::StatusCode::Good,
                  scada::StatusCode::Bad_WrongAttributeId},
  };
  const auto decoded = RoundTrip(12, response);
  const auto& typed = std::get<WriteResponse>(decoded.body);
  EXPECT_TRUE(typed.status.good());
  ASSERT_EQ(typed.results.size(), 2u);
  EXPECT_EQ(typed.results[0], scada::StatusCode::Good);
  EXPECT_EQ(typed.results[1], scada::StatusCode::Bad_WrongAttributeId);
}

TEST(OpcUaBinaryServiceCodecTest, BrowseResponseRoundTrip) {
  BrowseResponse response{
      .status = scada::StatusCode::Good,
      .results = {scada::BrowseResult{
                      .status_code = scada::StatusCode::Good,
                      .continuation_point = scada::ByteString{'c', 'p'},
                      .references = {scada::ReferenceDescription{
                          .reference_type_id = scada::NodeId{35},
                          .forward = true,
                          .node_id = scada::NodeId{100},
                      }},
                  },
                  scada::BrowseResult{
                      .status_code = scada::StatusCode::Bad_NothingToDo,
                  }},
  };
  const auto decoded = RoundTrip(13, response);
  const auto& typed = std::get<BrowseResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 2u);
  EXPECT_TRUE(scada::IsGood(typed.results[0].status_code));
  EXPECT_EQ(typed.results[0].continuation_point,
            response.results[0].continuation_point);
  ASSERT_EQ(typed.results[0].references.size(), 1u);
  EXPECT_EQ(typed.results[0].references[0].reference_type_id,
            scada::NodeId{35});
  EXPECT_TRUE(typed.results[0].references[0].forward);
  EXPECT_EQ(typed.results[0].references[0].node_id, scada::NodeId{100});
  EXPECT_EQ(typed.results[1].status_code, scada::StatusCode::Bad_NothingToDo);
  EXPECT_TRUE(typed.results[1].references.empty());
}

TEST(OpcUaBinaryServiceCodecTest, BrowseNextResponseRoundTrip) {
  BrowseNextResponse response{
      .status = scada::StatusCode::Good,
      .results = {scada::BrowseResult{.status_code = scada::StatusCode::Good}},
  };
  const auto decoded = RoundTrip(14, response);
  const auto& typed = std::get<BrowseNextResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 1u);
  EXPECT_TRUE(scada::IsGood(typed.results[0].status_code));
}

TEST(OpcUaBinaryServiceCodecTest, TranslateBrowsePathsResponseRoundTrip) {
  TranslateBrowsePathsResponse response{
      .status = scada::StatusCode::Good,
      .results = {scada::BrowsePathResult{
          .status_code = scada::StatusCode::Good,
          .targets = {scada::BrowsePathTarget{
              .target_id = scada::ExpandedNodeId{scada::NodeId{77}},
              .remaining_path_index = 0,
          }},
      }},
  };
  const auto decoded = RoundTrip(15, response);
  const auto& typed = std::get<TranslateBrowsePathsResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 1u);
  EXPECT_TRUE(scada::IsGood(typed.results[0].status_code));
  ASSERT_EQ(typed.results[0].targets.size(), 1u);
  EXPECT_EQ(typed.results[0].targets[0].target_id.node_id(),
            scada::NodeId{77});
}

TEST(OpcUaBinaryServiceCodecTest, CallResponseRoundTrip) {
  CallResponse response;
  response.results.push_back(
      {.status = scada::StatusCode::Good,
       .input_argument_results = {scada::StatusCode::Good},
       .output_arguments = {scada::Variant{std::int32_t{100}},
                            scada::Variant{std::int32_t{200}}}});
  const auto decoded = RoundTrip(16, response);
  const auto& typed = std::get<CallResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 1u);
  EXPECT_TRUE(typed.results[0].status.good());
  ASSERT_EQ(typed.results[0].input_argument_results.size(), 1u);
  EXPECT_EQ(typed.results[0].input_argument_results[0],
            scada::StatusCode::Good);
  ASSERT_EQ(typed.results[0].output_arguments.size(), 2u);
  EXPECT_EQ(typed.results[0].output_arguments[0],
            scada::Variant{std::int32_t{100}});
  EXPECT_EQ(typed.results[0].output_arguments[1],
            scada::Variant{std::int32_t{200}});
}

TEST(OpcUaBinaryServiceCodecTest, CreateSubscriptionResponseRoundTrip) {
  OpcUaCreateSubscriptionResponse response{
      .status = scada::StatusCode::Good,
      .subscription_id = 7,
      .revised_publishing_interval_ms = 500.0,
      .revised_lifetime_count = 1200,
      .revised_max_keep_alive_count = 20,
  };
  const auto decoded = RoundTrip(18, response);
  const auto& typed =
      std::get<OpcUaCreateSubscriptionResponse>(decoded.body);
  EXPECT_TRUE(typed.status.good());
  EXPECT_EQ(typed.subscription_id, 7u);
  EXPECT_EQ(typed.revised_publishing_interval_ms, 500.0);
  EXPECT_EQ(typed.revised_lifetime_count, 1200u);
  EXPECT_EQ(typed.revised_max_keep_alive_count, 20u);
}

TEST(OpcUaBinaryServiceCodecTest, ModifySubscriptionResponseRoundTrip) {
  OpcUaModifySubscriptionResponse response{
      .status = scada::StatusCode::Good,
      .revised_publishing_interval_ms = 1000.0,
      .revised_lifetime_count = 600,
      .revised_max_keep_alive_count = 10,
  };
  const auto decoded = RoundTrip(19, response);
  const auto& typed =
      std::get<OpcUaModifySubscriptionResponse>(decoded.body);
  EXPECT_TRUE(typed.status.good());
  EXPECT_EQ(typed.revised_publishing_interval_ms, 1000.0);
  EXPECT_EQ(typed.revised_lifetime_count, 600u);
  EXPECT_EQ(typed.revised_max_keep_alive_count, 10u);
}

TEST(OpcUaBinaryServiceCodecTest, DeleteSubscriptionsResponseRoundTrip) {
  OpcUaDeleteSubscriptionsResponse response{
      .status = scada::StatusCode::Good,
      .results = {scada::StatusCode::Good, scada::StatusCode::Bad},
  };
  const auto decoded = RoundTrip(20, response);
  const auto& typed =
      std::get<OpcUaDeleteSubscriptionsResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 2u);
  EXPECT_EQ(typed.results[0], scada::StatusCode::Good);
  EXPECT_EQ(typed.results[1], scada::StatusCode::Bad);
}

TEST(OpcUaBinaryServiceCodecTest, SetPublishingModeResponseRoundTrip) {
  OpcUaSetPublishingModeResponse response{
      .status = scada::StatusCode::Good,
      .results = {scada::StatusCode::Good},
  };
  const auto decoded = RoundTrip(21, response);
  const auto& typed =
      std::get<OpcUaSetPublishingModeResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 1u);
  EXPECT_EQ(typed.results[0], scada::StatusCode::Good);
}

TEST(OpcUaBinaryServiceCodecTest, CreateMonitoredItemsResponseRoundTrip) {
  OpcUaCreateMonitoredItemsResponse response{
      .status = scada::StatusCode::Good,
      .results = {OpcUaMonitoredItemCreateResult{
                      .status = scada::StatusCode::Good,
                      .monitored_item_id = 101,
                      .revised_sampling_interval_ms = 500.0,
                      .revised_queue_size = 8,
                  },
                  OpcUaMonitoredItemCreateResult{
                      .status = scada::StatusCode::Bad,
                      .monitored_item_id = 0,
                      .revised_sampling_interval_ms = 0.0,
                      .revised_queue_size = 0,
                  }},
  };
  const auto decoded = RoundTrip(22, response);
  const auto& typed =
      std::get<OpcUaCreateMonitoredItemsResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 2u);
  EXPECT_TRUE(typed.results[0].status.good());
  EXPECT_EQ(typed.results[0].monitored_item_id, 101u);
  EXPECT_EQ(typed.results[0].revised_sampling_interval_ms, 500.0);
  EXPECT_EQ(typed.results[0].revised_queue_size, 8u);
  EXPECT_TRUE(typed.results[1].status.bad());
}

TEST(OpcUaBinaryServiceCodecTest, ModifyMonitoredItemsResponseRoundTrip) {
  OpcUaModifyMonitoredItemsResponse response{
      .status = scada::StatusCode::Good,
      .results = {OpcUaMonitoredItemModifyResult{
          .status = scada::StatusCode::Good,
          .revised_sampling_interval_ms = 250.0,
          .revised_queue_size = 4,
      }},
  };
  const auto decoded = RoundTrip(23, response);
  const auto& typed =
      std::get<OpcUaModifyMonitoredItemsResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 1u);
  EXPECT_TRUE(typed.results[0].status.good());
  EXPECT_EQ(typed.results[0].revised_sampling_interval_ms, 250.0);
  EXPECT_EQ(typed.results[0].revised_queue_size, 4u);
}

TEST(OpcUaBinaryServiceCodecTest, DeleteMonitoredItemsResponseRoundTrip) {
  OpcUaDeleteMonitoredItemsResponse response{
      .status = scada::StatusCode::Good,
      .results = {scada::StatusCode::Good,
                  scada::StatusCode::Bad_MonitoredItemIdInvalid},
  };
  const auto decoded = RoundTrip(24, response);
  const auto& typed =
      std::get<OpcUaDeleteMonitoredItemsResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 2u);
  EXPECT_EQ(typed.results[0], scada::StatusCode::Good);
  EXPECT_EQ(typed.results[1], scada::StatusCode::Bad_MonitoredItemIdInvalid);
}

TEST(OpcUaBinaryServiceCodecTest, SetMonitoringModeResponseRoundTrip) {
  OpcUaSetMonitoringModeResponse response{
      .status = scada::StatusCode::Good,
      .results = {scada::StatusCode::Good},
  };
  const auto decoded = RoundTrip(25, response);
  const auto& typed =
      std::get<OpcUaSetMonitoringModeResponse>(decoded.body);
  ASSERT_EQ(typed.results.size(), 1u);
  EXPECT_EQ(typed.results[0], scada::StatusCode::Good);
}

TEST(OpcUaBinaryServiceCodecTest, PublishResponseRoundTripDataChange) {
  OpcUaPublishResponse response{
      .status = scada::StatusCode::Good,
      .subscription_id = 42,
      .results = {scada::StatusCode::Good},
      .more_notifications = false,
      .notification_message = {.sequence_number = 7,
                               .publish_time = base::Time{},
                               .notification_data =
                                   {OpcUaDataChangeNotification{
                                       .monitored_items =
                                           {{.client_handle = 1,
                                             .value = scada::DataValue{
                                                 scada::Variant{
                                                     std::int32_t{99}},
                                                 {}, {}, {}}}}}}},
      .available_sequence_numbers = {5, 6, 7},
  };
  const auto decoded = RoundTrip(26, response);
  const auto& typed = std::get<OpcUaPublishResponse>(decoded.body);
  EXPECT_EQ(typed.subscription_id, 42u);
  EXPECT_EQ(typed.available_sequence_numbers, response.available_sequence_numbers);
  EXPECT_FALSE(typed.more_notifications);
  EXPECT_EQ(typed.notification_message.sequence_number, 7u);
  ASSERT_EQ(typed.notification_message.notification_data.size(), 1u);
  const auto* change = std::get_if<OpcUaDataChangeNotification>(
      &typed.notification_message.notification_data[0]);
  ASSERT_NE(change, nullptr);
  ASSERT_EQ(change->monitored_items.size(), 1u);
  EXPECT_EQ(change->monitored_items[0].client_handle, 1u);
  EXPECT_EQ(change->monitored_items[0].value.value,
            scada::Variant{std::int32_t{99}});
}

TEST(OpcUaBinaryServiceCodecTest, PublishResponseRoundTripStatusChange) {
  OpcUaPublishResponse response{
      .status = scada::StatusCode::Good,
      .subscription_id = 1,
      .notification_message =
          {.sequence_number = 1,
           .notification_data = {OpcUaStatusChangeNotification{
               .status = scada::StatusCode::Bad_WrongSubscriptionId}}},
  };
  const auto decoded = RoundTrip(27, response);
  const auto& typed = std::get<OpcUaPublishResponse>(decoded.body);
  ASSERT_EQ(typed.notification_message.notification_data.size(), 1u);
  const auto* change = std::get_if<OpcUaStatusChangeNotification>(
      &typed.notification_message.notification_data[0]);
  ASSERT_NE(change, nullptr);
  EXPECT_EQ(change->status, scada::StatusCode::Bad_WrongSubscriptionId);
}

TEST(OpcUaBinaryServiceCodecTest, RepublishResponseRoundTrip) {
  OpcUaRepublishResponse response{
      .status = scada::StatusCode::Good,
      .notification_message = {.sequence_number = 3,
                               .notification_data = {}},
  };
  const auto decoded = RoundTrip(28, response);
  const auto& typed = std::get<OpcUaRepublishResponse>(decoded.body);
  EXPECT_TRUE(typed.status.good());
  EXPECT_EQ(typed.notification_message.sequence_number, 3u);
  EXPECT_TRUE(typed.notification_message.notification_data.empty());
}

TEST(OpcUaBinaryServiceCodecTest,
     DecodeResponseRejectsMalformedExtensionObject) {
  // Raw bytes that cannot be decoded as a valid ExtensionObject wrapper —
  // the dispatcher must refuse rather than misinterpret.
  EXPECT_FALSE(DecodeOpcUaBinaryServiceResponse(std::vector<char>{}).has_value());
  EXPECT_FALSE(
      DecodeOpcUaBinaryServiceResponse(std::vector<char>{0x00, 0x00, 0x00})
          .has_value());
}

TEST(OpcUaBinaryServiceCodecTest, DecodeResponseRejectsUnknownTypeId) {
  // Valid ExtensionObject wrapper but type id not in the response dispatch
  // table.
  std::vector<char> body;
  body.push_back(0x01);  // NodeId two-byte encoding
  body.push_back(0x00);
  body.push_back(0x55);  // numeric id 85 — not a response encoding id
  body.push_back(0x55);
  body.push_back(0x01);  // encoding = ByteString
  body.push_back(0x00);  // length = 0
  body.push_back(0x00);
  body.push_back(0x00);
  body.push_back(0x00);
  EXPECT_FALSE(DecodeOpcUaBinaryServiceResponse(body).has_value());
}

}  // namespace
}  // namespace opcua
