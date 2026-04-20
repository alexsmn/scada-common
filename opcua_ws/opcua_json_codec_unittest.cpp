#include "opcua_ws/opcua_json_codec.h"

#include "base/time_utils.h"
#include "scada/event.h"

#include <boost/json.hpp>
#include <boost/json/serialize.hpp>
#include <gtest/gtest.h>

using namespace testing;

namespace opcua_ws {
namespace {

scada::NodeId NumericNode(scada::NumericId id, scada::NamespaceIndex ns = 2) {
  return {id, ns};
}

base::Time ParseTime(std::string_view value) {
  base::Time result;
  EXPECT_TRUE(Deserialize(value, result));
  return result;
}

TEST(OpcUaJsonCodecTest, RoundTripsPhase0Requests) {
  ReadRequest read{
      .inputs = {{.node_id = NumericNode(1),
                  .attribute_id = scada::AttributeId::DisplayName}}};
  WriteRequest write{
      .inputs = {{.node_id = NumericNode(2),
                  .attribute_id = scada::AttributeId::Value,
                  .value = scada::Variant{std::vector<scada::UInt32>{4, 5}},
                  .flags = scada::WriteFlags{}.set_select().set_param()}}};
  BrowseRequest browse{
      .inputs = {{.node_id = NumericNode(3),
                  .direction = scada::BrowseDirection::Inverse,
                  .reference_type_id = NumericNode(31),
                  .include_subtypes = false}}};
  TranslateBrowsePathsRequest translate{
      .inputs = {{.node_id = NumericNode(4),
                  .relative_path = {{.reference_type_id = NumericNode(41),
                                     .inverse = true,
                                     .include_subtypes = false,
                                     .target_name = {"Child", 6}}}}}};

  const auto decoded_read = std::get<ReadRequest>(
      DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{read})));
  ASSERT_EQ(decoded_read.inputs.size(), 1u);
  EXPECT_EQ(decoded_read.inputs[0], read.inputs[0]);

  const auto decoded_write = std::get<WriteRequest>(
      DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{write})));
  ASSERT_EQ(decoded_write.inputs.size(), 1u);
  EXPECT_EQ(decoded_write.inputs[0], write.inputs[0]);

  const auto decoded_browse = std::get<BrowseRequest>(
      DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{browse})));
  ASSERT_EQ(decoded_browse.inputs.size(), 1u);
  EXPECT_EQ(decoded_browse.inputs[0], browse.inputs[0]);

  const auto decoded_translate = std::get<TranslateBrowsePathsRequest>(
      DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{translate})));
  ASSERT_EQ(decoded_translate.inputs.size(), 1u);
  EXPECT_EQ(decoded_translate.inputs[0], translate.inputs[0]);
}

TEST(OpcUaJsonCodecTest, RoundTripsSessionRequestMessages) {
  OpcUaWsRequestMessage create{
      .request_handle = 11,
      .body = OpcUaWsCreateSessionRequest{
          .requested_timeout = base::TimeDelta::FromSeconds(45)}};
  OpcUaWsRequestMessage activate{
      .request_handle = 12,
      .body = OpcUaWsActivateSessionRequest{
          .session_id = NumericNode(20),
          .authentication_token = NumericNode(21, 3),
          .user_name = scada::LocalizedText{u"operator"},
          .password = scada::LocalizedText{u"secret"},
          .delete_existing = true,
          .allow_anonymous = false,
      }};
  OpcUaWsRequestMessage close{
      .request_handle = 13,
      .body = OpcUaWsCloseSessionRequest{
          .session_id = NumericNode(22),
          .authentication_token = NumericNode(23, 3)}};

  const auto decoded_create = DecodeRequestMessage(EncodeJson(create));
  EXPECT_EQ(decoded_create.request_handle, create.request_handle);
  const auto* create_body =
      std::get_if<OpcUaWsCreateSessionRequest>(&decoded_create.body);
  ASSERT_NE(create_body, nullptr);
  EXPECT_EQ(create_body->requested_timeout,
            base::TimeDelta::FromSeconds(45));

  const auto decoded_activate = DecodeRequestMessage(EncodeJson(activate));
  EXPECT_EQ(decoded_activate.request_handle, activate.request_handle);
  const auto* activate_body =
      std::get_if<OpcUaWsActivateSessionRequest>(&decoded_activate.body);
  ASSERT_NE(activate_body, nullptr);
  EXPECT_EQ(activate_body->session_id, NumericNode(20));
  EXPECT_EQ(activate_body->authentication_token, NumericNode(21, 3));
  ASSERT_TRUE(activate_body->user_name.has_value());
  ASSERT_TRUE(activate_body->password.has_value());
  EXPECT_EQ(*activate_body->user_name, scada::LocalizedText{u"operator"});
  EXPECT_EQ(*activate_body->password, scada::LocalizedText{u"secret"});
  EXPECT_TRUE(activate_body->delete_existing);
  EXPECT_FALSE(activate_body->allow_anonymous);

  const auto decoded_close = DecodeRequestMessage(EncodeJson(close));
  EXPECT_EQ(decoded_close.request_handle, close.request_handle);
  const auto* close_body =
      std::get_if<OpcUaWsCloseSessionRequest>(&decoded_close.body);
  ASSERT_NE(close_body, nullptr);
  EXPECT_EQ(close_body->session_id, NumericNode(22));
  EXPECT_EQ(close_body->authentication_token, NumericNode(23, 3));
}

TEST(OpcUaJsonCodecTest, RoundTripsHistoryReadRawRequest) {
  HistoryReadRawRequest request{
      .details =
          {.node_id = NumericNode(1),
           .from = ParseTime("2026-04-19 10:00:00"),
           .to = ParseTime("2026-04-19 12:00:00"),
           .max_count = 123,
           .aggregation = {.start_time = ParseTime("2026-04-19 09:30:00"),
                           .interval = base::TimeDelta::FromMinutes(15),
                           .aggregate_type = NumericNode(44)},
           .release_continuation_point = true,
           .continuation_point = {'a', 'b', 'c'}}};

  auto json = EncodeJson(OpcUaWsServiceRequest{request});
  auto decoded = DecodeServiceRequest(json);
  EXPECT_NE(boost::json::serialize(json).find("HistoryReadRaw"),
            std::string::npos);
  const auto* typed = std::get_if<HistoryReadRawRequest>(&decoded);
  ASSERT_NE(typed, nullptr);
  EXPECT_EQ(typed->details.node_id, request.details.node_id);
  EXPECT_EQ(typed->details.from, request.details.from);
  EXPECT_EQ(typed->details.to, request.details.to);
  EXPECT_EQ(typed->details.max_count, request.details.max_count);
  EXPECT_EQ(typed->details.aggregation, request.details.aggregation);
  EXPECT_EQ(typed->details.release_continuation_point,
            request.details.release_continuation_point);
  EXPECT_EQ(typed->details.continuation_point, request.details.continuation_point);
}

TEST(OpcUaJsonCodecTest, RoundTripsHistoryReadEventsRequest) {
  HistoryReadEventsRequest request{
      .details =
          {.node_id = NumericNode(2),
           .from = ParseTime("2026-04-19 08:00:00"),
           .to = ParseTime("2026-04-19 09:00:00"),
           .filter =
               {.types = scada::EventFilter::UNACKED,
                .of_type = {NumericNode(77), NumericNode(78)},
                .child_of = {NumericNode(79)}}}};

  auto decoded = DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{request}));
  const auto* typed = std::get_if<HistoryReadEventsRequest>(&decoded);
  ASSERT_NE(typed, nullptr);
  EXPECT_EQ(typed->details.node_id, request.details.node_id);
  EXPECT_EQ(typed->details.from, request.details.from);
  EXPECT_EQ(typed->details.to, request.details.to);
  EXPECT_EQ(typed->details.filter, request.details.filter);
}

TEST(OpcUaJsonCodecTest, RoundTripsCallRequestWithScalarAndArrayVariants) {
  CallRequest request{
      .methods = {{.object_id = NumericNode(10),
                   .method_id = NumericNode(11),
                   .arguments =
                       {scada::Variant{true},
                        scada::Variant{std::vector<scada::Int32>{1, 2, 3}},
                        scada::Variant{scada::LocalizedText{u"hello"}},
                        scada::Variant{NumericNode(12)}}}}};

  auto decoded = DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{request}));
  const auto* typed = std::get_if<CallRequest>(&decoded);
  ASSERT_NE(typed, nullptr);
  ASSERT_EQ(typed->methods.size(), 1u);
  EXPECT_EQ(typed->methods[0].object_id, request.methods[0].object_id);
  EXPECT_EQ(typed->methods[0].method_id, request.methods[0].method_id);
  EXPECT_EQ(typed->methods[0].arguments, request.methods[0].arguments);
}

TEST(OpcUaJsonCodecTest, RoundTripsNodeManagementRequests) {
  AddNodesRequest add_nodes{
      .items = {{.requested_id = NumericNode(100),
                 .parent_id = NumericNode(101),
                 .node_class = scada::NodeClass::Variable,
                 .type_definition_id = NumericNode(102),
                 .attributes =
                     scada::NodeAttributes{}
                         .set_browse_name({"Pressure", 3})
                         .set_display_name(u"Pressure")
                         .set_data_type(NumericNode(103))
                         .set_value(scada::Variant{42.5})}}};
  DeleteNodesRequest delete_nodes{
      .items = {{.node_id = NumericNode(104), .delete_target_references = true}}};
  AddReferencesRequest add_refs{
      .items = {{.source_node_id = NumericNode(105),
                 .reference_type_id = NumericNode(106),
                 .forward = false,
                 .target_server_uri = "opc.tcp://server",
                 .target_node_id =
                     scada::ExpandedNodeId{NumericNode(107), "urn:test", 2},
                 .target_node_class = scada::NodeClass::Object}}};
  DeleteReferencesRequest delete_refs{
      .items = {{.source_node_id = NumericNode(108),
                 .reference_type_id = NumericNode(109),
                 .forward = true,
                 .target_node_id = scada::ExpandedNodeId{NumericNode(110)},
                 .delete_bidirectional = false}}};

  const auto decoded_add_nodes = std::get<AddNodesRequest>(
      DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{add_nodes})));
  ASSERT_EQ(decoded_add_nodes.items.size(), 1u);
  EXPECT_EQ(decoded_add_nodes.items[0], add_nodes.items[0]);

  const auto decoded_delete_nodes = std::get<DeleteNodesRequest>(
      DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{delete_nodes})));
  ASSERT_EQ(decoded_delete_nodes.items.size(), 1u);
  EXPECT_EQ(decoded_delete_nodes.items[0].node_id, delete_nodes.items[0].node_id);
  EXPECT_EQ(decoded_delete_nodes.items[0].delete_target_references,
            delete_nodes.items[0].delete_target_references);

  const auto decoded_add_refs = std::get<AddReferencesRequest>(
      DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{add_refs})));
  ASSERT_EQ(decoded_add_refs.items.size(), 1u);
  EXPECT_EQ(decoded_add_refs.items[0].source_node_id,
            add_refs.items[0].source_node_id);
  EXPECT_EQ(decoded_add_refs.items[0].reference_type_id,
            add_refs.items[0].reference_type_id);
  EXPECT_EQ(decoded_add_refs.items[0].forward, add_refs.items[0].forward);
  EXPECT_EQ(decoded_add_refs.items[0].target_server_uri,
            add_refs.items[0].target_server_uri);
  EXPECT_EQ(decoded_add_refs.items[0].target_node_id,
            add_refs.items[0].target_node_id);
  EXPECT_EQ(decoded_add_refs.items[0].target_node_class,
            add_refs.items[0].target_node_class);

  const auto decoded_delete_refs = std::get<DeleteReferencesRequest>(
      DecodeServiceRequest(EncodeJson(OpcUaWsServiceRequest{delete_refs})));
  ASSERT_EQ(decoded_delete_refs.items.size(), 1u);
  EXPECT_EQ(decoded_delete_refs.items[0].source_node_id,
            delete_refs.items[0].source_node_id);
  EXPECT_EQ(decoded_delete_refs.items[0].reference_type_id,
            delete_refs.items[0].reference_type_id);
  EXPECT_EQ(decoded_delete_refs.items[0].forward, delete_refs.items[0].forward);
  EXPECT_EQ(decoded_delete_refs.items[0].target_node_id,
            delete_refs.items[0].target_node_id);
  EXPECT_EQ(decoded_delete_refs.items[0].delete_bidirectional,
            delete_refs.items[0].delete_bidirectional);
}

TEST(OpcUaJsonCodecTest, RoundTripsHistoryReadResponses) {
  HistoryReadRawResponse raw{
      .result =
          {.status = scada::Status::FromFullCode(0x80030002u),
           .values = {scada::DataValue{scada::Variant{12.5},
                                       scada::Qualifier{scada::Qualifier::MANUAL},
                                       ParseTime("2026-04-19 12:00:00"),
                                       ParseTime("2026-04-19 12:00:01")}},
           .continuation_point = {'x', 'y'}}};

  scada::Event event;
  event.event_type_id = NumericNode(200);
  event.event_id = 201;
  event.time = ParseTime("2026-04-19 11:00:00");
  event.receive_time = ParseTime("2026-04-19 11:00:01");
  event.change_mask = scada::Event::EVT_VAL;
  event.severity = scada::kSeverityWarning;
  event.node_id = NumericNode(202);
  event.user_id = NumericNode(203);
  event.value = scada::Variant{std::string{"trip"}};
  event.qualifier = scada::Qualifier{scada::Qualifier::OFFLINE};
  event.message = u"Alarm";
  event.acked = true;
  event.acknowledged_time = ParseTime("2026-04-19 11:05:00");
  event.acknowledged_user_id = NumericNode(204);

  HistoryReadEventsResponse events{
      .result = {.status = scada::StatusCode::Good, .events = {event}}};

  EXPECT_EQ(std::get<HistoryReadRawResponse>(DecodeServiceResponse(
                EncodeJson(OpcUaWsServiceResponse{raw})))
                .result.status.full_code(),
            raw.result.status.full_code());
  EXPECT_EQ(std::get<HistoryReadRawResponse>(DecodeServiceResponse(
                EncodeJson(OpcUaWsServiceResponse{raw})))
                .result.values,
            raw.result.values);
  EXPECT_EQ(std::get<HistoryReadEventsResponse>(DecodeServiceResponse(
                EncodeJson(OpcUaWsServiceResponse{events})))
                .result.events,
            events.result.events);
}

TEST(OpcUaJsonCodecTest, RoundTripsPhase0Responses) {
  ReadResponse read{
      .status = scada::StatusCode::Good,
      .results = {scada::DataValue{scada::Variant{scada::LocalizedText{u"Pump"}},
                                   scada::Qualifier{scada::Qualifier::MANUAL},
                                   ParseTime("2026-04-19 10:10:00"),
                                   ParseTime("2026-04-19 10:10:01")}}};
  WriteResponse write{
      .status = scada::StatusCode::Bad_Disconnected,
      .results = {scada::StatusCode::Bad_Disconnected}};
  BrowseResponse browse{
      .status = scada::StatusCode::Good,
      .results = {{.status_code = scada::StatusCode::Good,
                   .references = {{.reference_type_id = NumericNode(301),
                                   .forward = false,
                                   .node_id = NumericNode(302)}}}}};
  TranslateBrowsePathsResponse translate{
      .status = scada::StatusCode::Good,
      .results = {{.status_code = scada::StatusCode::Good,
                   .targets = {{.target_id =
                                    scada::ExpandedNodeId{NumericNode(303),
                                                          "urn:test", 2},
                                .remaining_path_index = 1}}}}};

  const auto decoded_read = std::get<ReadResponse>(
      DecodeServiceResponse(EncodeJson(OpcUaWsServiceResponse{read})));
  EXPECT_EQ(decoded_read.status, read.status);
  EXPECT_EQ(decoded_read.results, read.results);

  const auto decoded_write = std::get<WriteResponse>(
      DecodeServiceResponse(EncodeJson(OpcUaWsServiceResponse{write})));
  EXPECT_EQ(decoded_write.status, write.status);
  EXPECT_EQ(decoded_write.results, write.results);

  const auto decoded_browse = std::get<BrowseResponse>(
      DecodeServiceResponse(EncodeJson(OpcUaWsServiceResponse{browse})));
  EXPECT_EQ(decoded_browse.status, browse.status);
  ASSERT_EQ(decoded_browse.results.size(), 1u);
  EXPECT_EQ(decoded_browse.results[0].status_code, browse.results[0].status_code);
  EXPECT_EQ(decoded_browse.results[0].references, browse.results[0].references);

  const auto decoded_translate = std::get<TranslateBrowsePathsResponse>(
      DecodeServiceResponse(EncodeJson(OpcUaWsServiceResponse{translate})));
  EXPECT_EQ(decoded_translate.status, translate.status);
  ASSERT_EQ(decoded_translate.results.size(), 1u);
  EXPECT_EQ(decoded_translate.results[0].status_code,
            translate.results[0].status_code);
  ASSERT_EQ(decoded_translate.results[0].targets.size(), 1u);
  EXPECT_EQ(decoded_translate.results[0].targets[0].target_id,
            translate.results[0].targets[0].target_id);
  EXPECT_EQ(decoded_translate.results[0].targets[0].remaining_path_index,
            translate.results[0].targets[0].remaining_path_index);
}

TEST(OpcUaJsonCodecTest, RoundTripsSessionResponseMessagesAndFault) {
  OpcUaWsResponseMessage create{
      .request_handle = 21,
      .body = OpcUaWsCreateSessionResponse{
          .status = scada::StatusCode::Good,
          .session_id = NumericNode(30),
          .authentication_token = NumericNode(31, 3),
          .server_nonce = {1, 2, 3, 4},
          .revised_timeout = base::TimeDelta::FromMinutes(5),
      }};
  OpcUaWsResponseMessage activate{
      .request_handle = 22,
      .body = OpcUaWsActivateSessionResponse{
          .status = scada::StatusCode::Bad_WrongLoginCredentials,
          .resumed = true,
      }};
  OpcUaWsResponseMessage close{
      .request_handle = 23,
      .body = OpcUaWsCloseSessionResponse{
          .status = scada::StatusCode::Good}};
  OpcUaWsResponseMessage fault{
      .request_handle = 24,
      .body = OpcUaWsServiceFault{
          .status = scada::StatusCode::Bad_Disconnected}};

  const auto decoded_create = DecodeResponseMessage(EncodeJson(create));
  EXPECT_EQ(decoded_create.request_handle, create.request_handle);
  const auto* create_body =
      std::get_if<OpcUaWsCreateSessionResponse>(&decoded_create.body);
  ASSERT_NE(create_body, nullptr);
  EXPECT_EQ(create_body->status, scada::StatusCode::Good);
  EXPECT_EQ(create_body->session_id, NumericNode(30));
  EXPECT_EQ(create_body->authentication_token, NumericNode(31, 3));
  EXPECT_EQ(create_body->server_nonce, (scada::ByteString{1, 2, 3, 4}));
  EXPECT_EQ(create_body->revised_timeout, base::TimeDelta::FromMinutes(5));

  const auto decoded_activate = DecodeResponseMessage(EncodeJson(activate));
  EXPECT_EQ(decoded_activate.request_handle, activate.request_handle);
  const auto* activate_body =
      std::get_if<OpcUaWsActivateSessionResponse>(&decoded_activate.body);
  ASSERT_NE(activate_body, nullptr);
  EXPECT_EQ(activate_body->status.code(),
            scada::StatusCode::Bad_WrongLoginCredentials);
  EXPECT_TRUE(activate_body->resumed);

  const auto decoded_close = DecodeResponseMessage(EncodeJson(close));
  EXPECT_EQ(decoded_close.request_handle, close.request_handle);
  const auto* close_body =
      std::get_if<OpcUaWsCloseSessionResponse>(&decoded_close.body);
  ASSERT_NE(close_body, nullptr);
  EXPECT_EQ(close_body->status.code(), scada::StatusCode::Good);

  const auto decoded_fault = DecodeResponseMessage(EncodeJson(fault));
  EXPECT_EQ(decoded_fault.request_handle, fault.request_handle);
  const auto* fault_body = std::get_if<OpcUaWsServiceFault>(&decoded_fault.body);
  ASSERT_NE(fault_body, nullptr);
  EXPECT_EQ(fault_body->status.code(), scada::StatusCode::Bad_Disconnected);
}

TEST(OpcUaJsonCodecTest, RoundTripsServiceMessagesWithEnvelope) {
  OpcUaWsRequestMessage request{
      .request_handle = 31,
      .body = BrowseRequest{
          .inputs = {{.node_id = NumericNode(40),
                      .direction = scada::BrowseDirection::Forward,
                      .reference_type_id = NumericNode(41),
                      .include_subtypes = true}}}};
  OpcUaWsResponseMessage response{
      .request_handle = 31,
      .body = BrowseResponse{
          .status = scada::StatusCode::Good,
          .results = {{.status_code = scada::StatusCode::Good,
                       .references = {{.reference_type_id = NumericNode(42),
                                       .forward = true,
                                       .node_id = NumericNode(43)}}}}}};

  const auto decoded_request = DecodeRequestMessage(EncodeJson(request));
  EXPECT_EQ(decoded_request.request_handle, request.request_handle);
  const auto* request_body = std::get_if<BrowseRequest>(&decoded_request.body);
  ASSERT_NE(request_body, nullptr);
  ASSERT_EQ(request_body->inputs.size(), 1u);
  EXPECT_EQ(request_body->inputs[0].node_id, NumericNode(40));
  EXPECT_EQ(request_body->inputs[0].reference_type_id, NumericNode(41));

  const auto decoded_response = DecodeResponseMessage(EncodeJson(response));
  EXPECT_EQ(decoded_response.request_handle, response.request_handle);
  const auto* response_body = std::get_if<BrowseResponse>(&decoded_response.body);
  ASSERT_NE(response_body, nullptr);
  EXPECT_EQ(response_body->status.code(), scada::StatusCode::Good);
  ASSERT_EQ(response_body->results.size(), 1u);
  ASSERT_EQ(response_body->results[0].references.size(), 1u);
  EXPECT_EQ(response_body->results[0].references[0].node_id, NumericNode(43));
}

TEST(OpcUaJsonCodecTest, RoundTripsSubscriptionLifecycleRequestMessages) {
  OpcUaWsRequestMessage create_subscription{
      .request_handle = 41,
      .body = OpcUaWsCreateSubscriptionRequest{
          .parameters =
              {.publishing_interval_ms = 1000,
               .lifetime_count = 60,
               .max_keep_alive_count = 10,
               .max_notifications_per_publish = 0,
               .publishing_enabled = true,
               .priority = 1}}};
  OpcUaWsRequestMessage modify_subscription{
      .request_handle = 42,
      .body = OpcUaWsModifySubscriptionRequest{
          .subscription_id = 17,
          .parameters =
              {.publishing_interval_ms = 250,
               .lifetime_count = 30,
               .max_keep_alive_count = 5,
               .max_notifications_per_publish = 100,
               .publishing_enabled = true,
               .priority = 2}}};
  OpcUaWsRequestMessage set_publishing_mode{
      .request_handle = 43,
      .body = OpcUaWsSetPublishingModeRequest{
          .publishing_enabled = false,
          .subscription_ids = {17, 18}}};
  OpcUaWsRequestMessage delete_subscriptions{
      .request_handle = 44,
      .body = OpcUaWsDeleteSubscriptionsRequest{.subscription_ids = {19}}};

  const auto create_decoded =
      DecodeRequestMessage(EncodeJson(create_subscription));
  EXPECT_EQ(create_decoded.request_handle, 41u);
  EXPECT_EQ(std::get<OpcUaWsCreateSubscriptionRequest>(create_decoded.body)
                .parameters.max_keep_alive_count,
            10u);

  const auto modify_decoded =
      DecodeRequestMessage(EncodeJson(modify_subscription));
  EXPECT_EQ(std::get<OpcUaWsModifySubscriptionRequest>(modify_decoded.body)
                .subscription_id,
            17u);

  const auto set_mode_decoded =
      DecodeRequestMessage(EncodeJson(set_publishing_mode));
  const auto& set_mode_body =
      std::get<OpcUaWsSetPublishingModeRequest>(set_mode_decoded.body);
  EXPECT_FALSE(set_mode_body.publishing_enabled);
  EXPECT_EQ(set_mode_body.subscription_ids,
            (std::vector<OpcUaWsSubscriptionId>{17u, 18u}));

  const auto delete_decoded =
      DecodeRequestMessage(EncodeJson(delete_subscriptions));
  EXPECT_EQ(std::get<OpcUaWsDeleteSubscriptionsRequest>(delete_decoded.body)
                .subscription_ids,
            (std::vector<OpcUaWsSubscriptionId>{19u}));
}

TEST(OpcUaJsonCodecTest, RoundTripsMonitoredItemLifecycleMessages) {
  const boost::json::value raw_event_filter =
      boost::json::parse(R"({"select":["Message"],"where":{"severity":60}})");
  OpcUaWsRequestMessage create_items{
      .request_handle = 51,
      .body = OpcUaWsCreateMonitoredItemsRequest{
          .subscription_id = 17,
          .timestamps_to_return = OpcUaWsTimestampsToReturn::Both,
          .items_to_create =
              {{.item_to_monitor =
                    {.node_id = NumericNode(70),
                     .attribute_id = scada::AttributeId::Value},
                .monitoring_mode = OpcUaWsMonitoringMode::Reporting,
                .requested_parameters =
                    {.client_handle = 1,
                     .sampling_interval_ms = 250,
                     .filter = OpcUaWsMonitoringFilter{
                         OpcUaWsDataChangeFilter{
                             .trigger =
                                 OpcUaWsDataChangeTrigger::StatusValue,
                             .deadband_type =
                                 OpcUaWsDeadbandType::Absolute,
                             .deadband_value = 0.5}},
                     .queue_size = 4,
                     .discard_oldest = true}},
               {.item_to_monitor =
                    {.node_id = NumericNode(71),
                     .attribute_id = scada::AttributeId::EventNotifier},
                .index_range = "0:10",
                .monitoring_mode = OpcUaWsMonitoringMode::Sampling,
                .requested_parameters =
                    {.client_handle = 2,
                     .sampling_interval_ms = 1000,
                     .filter = OpcUaWsMonitoringFilter{raw_event_filter},
                     .queue_size = 1,
                     .discard_oldest = false}}}}};
  OpcUaWsRequestMessage modify_items{
      .request_handle = 52,
      .body = OpcUaWsModifyMonitoredItemsRequest{
          .subscription_id = 17,
          .timestamps_to_return = OpcUaWsTimestampsToReturn::Source,
          .items_to_modify =
              {{.monitored_item_id = 42,
                .requested_parameters =
                    {.client_handle = 1,
                     .sampling_interval_ms = 1000,
                     .queue_size = 8,
                     .discard_oldest = false}}}}};
  OpcUaWsRequestMessage delete_items{
      .request_handle = 53,
      .body = OpcUaWsDeleteMonitoredItemsRequest{
          .subscription_id = 17,
          .monitored_item_ids = {42, 43}}};
  OpcUaWsRequestMessage set_monitoring_mode{
      .request_handle = 54,
      .body = OpcUaWsSetMonitoringModeRequest{
          .subscription_id = 17,
          .monitoring_mode = OpcUaWsMonitoringMode::Disabled,
          .monitored_item_ids = {42}}};

  const auto create_decoded = DecodeRequestMessage(EncodeJson(create_items));
  const auto& create_body =
      std::get<OpcUaWsCreateMonitoredItemsRequest>(create_decoded.body);
  EXPECT_EQ(create_body.subscription_id, 17u);
  ASSERT_EQ(create_body.items_to_create.size(), 2u);
  EXPECT_EQ(create_body.items_to_create[0].requested_parameters.queue_size, 4u);
  ASSERT_TRUE(create_body.items_to_create[0].requested_parameters.filter.has_value());
  const auto* first_filter = std::get_if<OpcUaWsDataChangeFilter>(
      &*create_body.items_to_create[0].requested_parameters.filter);
  ASSERT_NE(first_filter, nullptr);
  EXPECT_EQ(first_filter->deadband_value, 0.5);
  ASSERT_TRUE(create_body.items_to_create[1].requested_parameters.filter.has_value());
  const auto* second_filter = std::get_if<boost::json::value>(
      &*create_body.items_to_create[1].requested_parameters.filter);
  ASSERT_NE(second_filter, nullptr);
  EXPECT_EQ(*second_filter, raw_event_filter);

  const auto modify_decoded = DecodeRequestMessage(EncodeJson(modify_items));
  EXPECT_EQ(std::get<OpcUaWsModifyMonitoredItemsRequest>(modify_decoded.body)
                .items_to_modify[0]
                .requested_parameters.sampling_interval_ms,
            1000);

  const auto delete_decoded = DecodeRequestMessage(EncodeJson(delete_items));
  EXPECT_EQ(std::get<OpcUaWsDeleteMonitoredItemsRequest>(delete_decoded.body)
                .monitored_item_ids,
            (std::vector<OpcUaWsMonitoredItemId>{42u, 43u}));

  const auto set_mode_decoded =
      DecodeRequestMessage(EncodeJson(set_monitoring_mode));
  EXPECT_EQ(std::get<OpcUaWsSetMonitoringModeRequest>(set_mode_decoded.body)
                .monitoring_mode,
            OpcUaWsMonitoringMode::Disabled);
}

TEST(OpcUaJsonCodecTest, RoundTripsSubscriptionLifecycleResponses) {
  const boost::json::value filter_result =
      boost::json::parse(R"({"kind":"event","selectClauseResults":[0]})");
  OpcUaWsResponseMessage create_subscription{
      .request_handle = 61,
      .body = OpcUaWsCreateSubscriptionResponse{
          .status = scada::StatusCode::Good,
          .subscription_id = 17,
          .revised_publishing_interval_ms = 1000,
          .revised_lifetime_count = 60,
          .revised_max_keep_alive_count = 10}};
  OpcUaWsResponseMessage modify_subscription{
      .request_handle = 62,
      .body = OpcUaWsModifySubscriptionResponse{
          .status = scada::StatusCode::Good,
          .revised_publishing_interval_ms = 250,
          .revised_lifetime_count = 30,
          .revised_max_keep_alive_count = 5}};
  OpcUaWsResponseMessage set_publishing_mode{
      .request_handle = 63,
      .body = OpcUaWsSetPublishingModeResponse{
          .status = scada::StatusCode::Good,
          .results = {scada::StatusCode::Good,
                      scada::StatusCode::Bad_WrongSubscriptionId}}};
  OpcUaWsResponseMessage create_items{
      .request_handle = 64,
      .body = OpcUaWsCreateMonitoredItemsResponse{
          .status = scada::StatusCode::Good,
          .results =
              {{.status = scada::StatusCode::Good,
                .monitored_item_id = 42,
                .revised_sampling_interval_ms = 250,
                .revised_queue_size = 1},
               {.status = scada::StatusCode::Bad_WrongNodeId,
                .monitored_item_id = 0,
                .revised_sampling_interval_ms = 1000,
                .revised_queue_size = 4,
                .filter_result = filter_result}}}};
  OpcUaWsResponseMessage modify_items{
      .request_handle = 65,
      .body = OpcUaWsModifyMonitoredItemsResponse{
          .status = scada::StatusCode::Good,
          .results = {{.status = scada::StatusCode::Good,
                       .revised_sampling_interval_ms = 500,
                       .revised_queue_size = 8}}}};
  OpcUaWsResponseMessage delete_items{
      .request_handle = 66,
      .body = OpcUaWsDeleteMonitoredItemsResponse{
          .status = scada::StatusCode::Good,
          .results = {scada::StatusCode::Good}}};
  OpcUaWsResponseMessage set_monitoring_mode{
      .request_handle = 67,
      .body = OpcUaWsSetMonitoringModeResponse{
          .status = scada::StatusCode::Good,
          .results = {scada::StatusCode::Good,
                      scada::StatusCode::Bad_WrongSubscriptionId}}};

  EXPECT_EQ(std::get<OpcUaWsCreateSubscriptionResponse>(
                DecodeResponseMessage(EncodeJson(create_subscription)).body)
                .subscription_id,
            17u);
  EXPECT_EQ(std::get<OpcUaWsModifySubscriptionResponse>(
                DecodeResponseMessage(EncodeJson(modify_subscription)).body)
                .revised_max_keep_alive_count,
            5u);
  EXPECT_EQ(std::get<OpcUaWsSetPublishingModeResponse>(
                DecodeResponseMessage(EncodeJson(set_publishing_mode)).body)
                .results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good,
                                            scada::StatusCode::Bad_WrongSubscriptionId}));

  const auto decoded_create_items =
      DecodeResponseMessage(EncodeJson(create_items));
  const auto& create_items_body =
      std::get<OpcUaWsCreateMonitoredItemsResponse>(
          decoded_create_items.body);
  ASSERT_EQ(create_items_body.results.size(), 2u);
  ASSERT_TRUE(create_items_body.results[1].filter_result.has_value());
  EXPECT_EQ(*create_items_body.results[1].filter_result, filter_result);

  EXPECT_EQ(std::get<OpcUaWsModifyMonitoredItemsResponse>(
                DecodeResponseMessage(EncodeJson(modify_items)).body)
                .results[0]
                .revised_queue_size,
            8u);
  EXPECT_EQ(std::get<OpcUaWsDeleteMonitoredItemsResponse>(
                DecodeResponseMessage(EncodeJson(delete_items)).body)
                .results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));
  EXPECT_EQ(std::get<OpcUaWsSetMonitoringModeResponse>(
                DecodeResponseMessage(EncodeJson(set_monitoring_mode)).body)
                .results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good,
                                            scada::StatusCode::Bad_WrongSubscriptionId}));
}

TEST(OpcUaJsonCodecTest, RoundTripsPublishAndRecoveryRequestMessages) {
  OpcUaWsRequestMessage publish{
      .request_handle = 71,
      .body = OpcUaWsPublishRequest{
          .subscription_acknowledgements =
              {{.subscription_id = 17, .sequence_number = 3},
               {.subscription_id = 18, .sequence_number = 7}}}};
  OpcUaWsRequestMessage republish{
      .request_handle = 72,
      .body = OpcUaWsRepublishRequest{
          .subscription_id = 17, .retransmit_sequence_number = 5}};
  OpcUaWsRequestMessage transfer{
      .request_handle = 73,
      .body = OpcUaWsTransferSubscriptionsRequest{
          .subscription_ids = {17, 18}, .send_initial_values = true}};
  const std::vector<OpcUaWsSubscriptionAcknowledgement> expected_acks{
      {.subscription_id = 17, .sequence_number = 3},
      {.subscription_id = 18, .sequence_number = 7}};
  const std::vector<OpcUaWsSubscriptionId> expected_transfer_ids{17u, 18u};

  const auto publish_decoded = DecodeRequestMessage(EncodeJson(publish));
  EXPECT_EQ(publish_decoded.request_handle, 71u);
  EXPECT_EQ(std::get<OpcUaWsPublishRequest>(publish_decoded.body)
                .subscription_acknowledgements,
            expected_acks);

  const auto republish_decoded = DecodeRequestMessage(EncodeJson(republish));
  EXPECT_EQ(std::get<OpcUaWsRepublishRequest>(republish_decoded.body)
                .retransmit_sequence_number,
            5u);

  const auto transfer_decoded = DecodeRequestMessage(EncodeJson(transfer));
  const auto& transfer_body =
      std::get<OpcUaWsTransferSubscriptionsRequest>(transfer_decoded.body);
  EXPECT_EQ(transfer_body.subscription_ids, expected_transfer_ids);
  EXPECT_TRUE(transfer_body.send_initial_values);
}

TEST(OpcUaJsonCodecTest, RoundTripsPublishAndRecoveryResponses) {
  const auto publish_time = ParseTime("2026-04-19 00:00:05");
  scada::DataValue republish_value;
  republish_value.value = scada::Variant{true};
  OpcUaWsNotificationMessage publish_message{
      .sequence_number = 3,
      .publish_time = publish_time,
      .notification_data =
          {OpcUaWsDataChangeNotification{
               .monitored_items =
                   {{.client_handle = 1,
                     .value = scada::DataValue{
                         scada::Variant{42.5},
                         scada::Qualifier{scada::Qualifier::MANUAL},
                         ParseTime("2026-04-19 00:00:05"),
                         ParseTime("2026-04-19 00:00:06")}}}},
           OpcUaWsEventNotificationList{
               .events = {{.client_handle = 2,
                           .event_fields =
                               {scada::Variant{std::string{"AlarmRaised"}},
                                scada::Variant{scada::UInt32{500}}}}}},
           OpcUaWsStatusChangeNotification{
               .status = scada::StatusCode::Bad_Timeout}}};
  OpcUaWsResponseMessage publish{
      .request_handle = 81,
      .body = OpcUaWsPublishResponse{
          .status = scada::StatusCode::Good,
          .subscription_id = 17,
          .available_sequence_numbers = {3, 4},
          .more_notifications = true,
          .notification_message = publish_message,
          .results = {scada::StatusCode::Good}}};
  OpcUaWsResponseMessage republish{
      .request_handle = 82,
      .body = OpcUaWsRepublishResponse{
          .status = scada::StatusCode::Good,
          .notification_message =
              {.sequence_number = 5,
               .publish_time = ParseTime("2026-04-19 00:00:07"),
               .notification_data = {OpcUaWsDataChangeNotification{
                   .monitored_items = {{.client_handle = 9,
                                        .value = republish_value}}}}}}};
  OpcUaWsResponseMessage transfer{
      .request_handle = 83,
      .body = OpcUaWsTransferSubscriptionsResponse{
          .status = scada::StatusCode::Good,
          .results = {scada::StatusCode::Good,
                      scada::StatusCode::Bad_WrongSubscriptionId}}};

  const auto decoded_publish = DecodeResponseMessage(EncodeJson(publish));
  EXPECT_EQ(decoded_publish.request_handle, 81u);
  const auto& publish_body =
      std::get<OpcUaWsPublishResponse>(decoded_publish.body);
  EXPECT_EQ(publish_body.subscription_id, 17u);
  EXPECT_EQ(publish_body.available_sequence_numbers,
            (std::vector<scada::UInt32>{3u, 4u}));
  EXPECT_TRUE(publish_body.more_notifications);
  EXPECT_EQ(publish_body.notification_message.sequence_number, 3u);
  EXPECT_EQ(publish_body.notification_message.publish_time, publish_time);
  ASSERT_EQ(publish_body.notification_message.notification_data.size(), 3u);
  const auto* data_change = std::get_if<OpcUaWsDataChangeNotification>(
      &publish_body.notification_message.notification_data[0]);
  ASSERT_NE(data_change, nullptr);
  ASSERT_EQ(data_change->monitored_items.size(), 1u);
  EXPECT_EQ(data_change->monitored_items[0].client_handle, 1u);
  EXPECT_EQ(data_change->monitored_items[0].value.value.get<double>(), 42.5);
  const auto* events = std::get_if<OpcUaWsEventNotificationList>(
      &publish_body.notification_message.notification_data[1]);
  ASSERT_NE(events, nullptr);
  ASSERT_EQ(events->events.size(), 1u);
  EXPECT_EQ(events->events[0].client_handle, 2u);
  EXPECT_EQ(events->events[0].event_fields.size(), 2u);
  const auto* status_change = std::get_if<OpcUaWsStatusChangeNotification>(
      &publish_body.notification_message.notification_data[2]);
  ASSERT_NE(status_change, nullptr);
  EXPECT_EQ(status_change->status, scada::StatusCode::Bad_Timeout);
  EXPECT_EQ(publish_body.results,
            (std::vector<scada::StatusCode>{scada::StatusCode::Good}));

  const auto decoded_republish = DecodeResponseMessage(EncodeJson(republish));
  EXPECT_EQ(std::get<OpcUaWsRepublishResponse>(decoded_republish.body)
                .notification_message.sequence_number,
            5u);

  const auto decoded_transfer = DecodeResponseMessage(EncodeJson(transfer));
  EXPECT_EQ(std::get<OpcUaWsTransferSubscriptionsResponse>(
                decoded_transfer.body)
                .results,
            (std::vector<scada::StatusCode>{
                scada::StatusCode::Good,
                scada::StatusCode::Bad_WrongSubscriptionId}));
}

TEST(OpcUaJsonCodecTest, RoundTripsCallAndMutationResponses) {
  CallResponse call{.results = {{.status = scada::StatusCode::Good},
                                {.status = scada::StatusCode::Bad_WrongCallArguments}}};
  AddNodesResponse add_nodes{
      .status = scada::StatusCode::Good,
      .results = {{.status_code = scada::StatusCode::Good,
                   .added_node_id = NumericNode(300)}}};
  DeleteNodesResponse delete_nodes{
      .status = scada::StatusCode::Bad_Disconnected,
      .results = {scada::StatusCode::Bad_Disconnected}};
  AddReferencesResponse add_refs{
      .status = scada::StatusCode::Good,
      .results = {scada::StatusCode::Good, scada::StatusCode::Bad_WrongTargetId}};
  DeleteReferencesResponse delete_refs{
      .status = scada::StatusCode::Good,
      .results = {scada::StatusCode::Good}};

  EXPECT_EQ(std::get<CallResponse>(DecodeServiceResponse(
                EncodeJson(OpcUaWsServiceResponse{call})))
                .results.size(),
            call.results.size());
  const auto decoded_add_nodes = std::get<AddNodesResponse>(
      DecodeServiceResponse(EncodeJson(OpcUaWsServiceResponse{add_nodes})));
  EXPECT_EQ(decoded_add_nodes.status, add_nodes.status);
  ASSERT_EQ(decoded_add_nodes.results.size(), 1u);
  EXPECT_EQ(decoded_add_nodes.results[0].status_code,
            add_nodes.results[0].status_code);
  EXPECT_EQ(decoded_add_nodes.results[0].added_node_id,
            add_nodes.results[0].added_node_id);
  EXPECT_EQ(std::get<DeleteNodesResponse>(DecodeServiceResponse(
                EncodeJson(OpcUaWsServiceResponse{delete_nodes})))
                .results,
            delete_nodes.results);
  EXPECT_EQ(std::get<AddReferencesResponse>(DecodeServiceResponse(
                EncodeJson(OpcUaWsServiceResponse{add_refs})))
                .results,
            add_refs.results);
  EXPECT_EQ(std::get<DeleteReferencesResponse>(DecodeServiceResponse(
                EncodeJson(OpcUaWsServiceResponse{delete_refs})))
                .results,
            delete_refs.results);
}

TEST(OpcUaJsonCodecTest, RejectsUnknownService) {
  boost::json::value json = boost::json::object{{"service", "Unknown"},
                                                {"body", boost::json::object{}}};
  EXPECT_THROW((void)DecodeServiceRequest(json), std::runtime_error);
  EXPECT_THROW((void)DecodeServiceResponse(json), std::runtime_error);
}

}  // namespace
}  // namespace opcua_ws
