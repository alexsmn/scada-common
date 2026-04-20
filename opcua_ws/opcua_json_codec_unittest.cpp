#include "opcua_ws/opcua_json_codec.h"

#include "base/time_utils.h"
#include "scada/event.h"

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
