#include "opcua_bridge/conversion.h"
#include "opcua_bridge/service_conversion.h"

#include "scada/authorization.h"
#include "scada/extension_object.h"
#include "scada/identity_mapping_rule_encoding.h"

#include "opcua/transport/binary/codec_utils.h"

#include <any>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

namespace scada::opcua_bridge {
namespace {

// Round-trips a scada value through opcua and back; the result must equal the
// original. This simultaneously exercises both conversion directions and
// proves the two type universes coexist in one translation unit.
template <class T>
void ExpectRoundTrip(const T& original) {
  EXPECT_EQ(ToScada(ToOpcua(original)), original);
}

TEST(ConversionTest, NodeId) {
  ExpectRoundTrip(scada::NodeId{42u});
  ExpectRoundTrip(scada::NodeId{1234u, 3});
  ExpectRoundTrip(scada::NodeId{scada::String{"the.node"}, 2});
  ExpectRoundTrip(scada::NodeId{scada::ByteString{'a', 'b', 'c'}, 1});
  ExpectRoundTrip(scada::NodeId{});
}

TEST(ConversionTest, RolePermissionTypeExtensionObjectEncodesBinaryBody) {
  const scada::RolePermissionType role{
      .role_id = scada::NodeId{15680u, 0},  // WellKnownRole_Operator
      .permissions = scada::Permission::kRead | scada::Permission::kWrite};
  const scada::ExtensionObject scada_object{
      scada::ExpandedNodeId{scada::NodeId{96u, 0}}, std::any{role}};

  const opcua::ExtensionObject opcua_object = ToOpcua(scada_object);

  // The wire encoding id is RolePermissionType_Encoding_DefaultBinary (i=128).
  EXPECT_EQ(opcua_object.data_type_id().node_id().numeric_id(), 128u);

  // The body decodes back to the original role id and permission bits.
  const auto* body = std::any_cast<std::vector<char>>(&opcua_object.value());
  ASSERT_NE(body, nullptr);
  opcua::binary::Decoder decoder{*body};
  opcua::NodeId decoded_role_id;
  std::uint32_t decoded_permissions = 0;
  ASSERT_TRUE(decoder.Decode(decoded_role_id));
  ASSERT_TRUE(decoder.Decode(decoded_permissions));
  EXPECT_EQ(decoded_role_id.numeric_id(), 15680u);
  EXPECT_EQ(decoded_permissions,
            static_cast<std::uint32_t>(scada::Permission::kRead |
                                       scada::Permission::kWrite));
}

// An IdentityMappingRuleType payload survives the boundary in BOTH directions:
// the encode side emits the DefaultBinary body (CriteriaType Int32 + Criteria
// String per the official 1.05 Opc.Ua.Types.bsd.xml), and the decode side
// reconstructs the typed rule from that body — so AddIdentity/RemoveIdentity
// method arguments (OPC UA Part 18 §4.4.5/§4.4.6) arrive structured.
TEST(ConversionTest, IdentityMappingRuleExtensionObjectRoundTrips) {
  const scada::IdentityMappingRule rule{
      .criteria_type = scada::IdentityCriteriaType::kUserName,
      .criteria = "svc-site-a"};
  const scada::ExtensionObject scada_object =
      scada::MakeIdentityMappingRuleObject(rule);

  const opcua::ExtensionObject opcua_object = ToOpcua(scada_object);

  // The wire encoding id is IdentityMappingRuleType_Encoding_DefaultBinary.
  EXPECT_EQ(opcua_object.data_type_id().node_id().numeric_id(),
            scada::kIdentityMappingRuleTypeDefaultBinaryId);

  const scada::ExtensionObject decoded = ToScada(opcua_object);
  EXPECT_EQ(decoded.data_type_id().node_id().numeric_id(),
            scada::kIdentityMappingRuleTypeDataTypeId);
  const auto decoded_rule = scada::DecodeIdentityMappingRule(decoded);
  ASSERT_TRUE(decoded_rule.has_value());
  EXPECT_EQ(*decoded_rule, rule);
}

TEST(ConversionTest, QualifiedName) {
  ExpectRoundTrip(scada::QualifiedName{"Temperature", 4});
  ExpectRoundTrip(scada::QualifiedName{});
}

TEST(ConversionTest, ExpandedNodeId) {
  ExpectRoundTrip(scada::ExpandedNodeId{scada::NodeId{7u, 1}});
}

TEST(ConversionTest, StatusAndCode) {
  EXPECT_EQ(ToScada(ToOpcua(scada::StatusCode::Bad_WrongNodeId)),
            scada::StatusCode::Bad_WrongNodeId);
  ExpectRoundTrip(scada::Status{scada::StatusCode::Good});
  ExpectRoundTrip(scada::Status{scada::StatusCode::Bad_WrongLoginCredentials});
}

// The boundary translates SCADA-internal status codes to the standard OPC UA
// StatusCode values that opcuapp puts on the wire (Opc.Ua.StatusCodes.csv).
TEST(ConversionTest, StatusCodeMapsToStandardOpcUaWireValue) {
  const auto wire = [](scada::StatusCode code) {
    return opcua::Status{ToOpcua(code)}.full_code();
  };
  EXPECT_EQ(wire(scada::StatusCode::Bad_NothingToDo), 0x800F0000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_TooManyOperations), 0x80100000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_WrongNodeId),
            0x80340000u);  // NodeIdUnknown
  EXPECT_EQ(wire(scada::StatusCode::Bad_ViewIdUnknown), 0x806B0000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_TimestampsToReturnInvalid),
            0x802B0000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_NoSubscription), 0x80790000u);
  EXPECT_EQ(wire(scada::StatusCode::Bad_HistoryOperationInvalid), 0x80710000u);
  // Good is unchanged.
  EXPECT_EQ(wire(scada::StatusCode::Good), 0x00000000u);

  // Every mapped code round-trips back to the original core code.
  for (const auto code : {scada::StatusCode::Bad_NothingToDo,
                          scada::StatusCode::Bad_ViewIdUnknown,
                          scada::StatusCode::Bad_NoSubscription,
                          scada::StatusCode::Bad_WrongSubscriptionId,
                          scada::StatusCode::Bad_Disconnected}) {
    EXPECT_EQ(ToScada(ToOpcua(code)), code);
  }
}

TEST(ConversionTest, DateTime) {
  const auto scada_time = base::Time::FromInternalValue(123456789);
  EXPECT_EQ(ToOpcua(scada_time).ToInternalValue(), 1234567890);
  ExpectRoundTrip(scada_time);
  ExpectRoundTrip(base::Time{});
  ExpectRoundTrip(base::Time::Min());
  ExpectRoundTrip(base::Time::Max());
}

TEST(ConversionTest, Duration) {
  const auto scada_duration = base::TimeDelta::FromMicroseconds(1250);
  EXPECT_DOUBLE_EQ(ToOpcua(scada_duration).ToInternalValue(), 1.25);
  ExpectRoundTrip(scada_duration);
  ExpectRoundTrip(base::TimeDelta::Min());
  ExpectRoundTrip(base::TimeDelta::Max());
}

TEST(ConversionTest, VariantScalars) {
  ExpectRoundTrip(scada::Variant{true});
  ExpectRoundTrip(scada::Variant{scada::Int32{-7}});
  ExpectRoundTrip(scada::Variant{scada::UInt64{99}});
  ExpectRoundTrip(scada::Variant{scada::Double{3.5}});
  ExpectRoundTrip(scada::Variant{scada::String{"hello"}});
  ExpectRoundTrip(scada::Variant{scada::NodeId{55u, 2}});
  ExpectRoundTrip(scada::Variant{scada::QualifiedName{"q", 1}});
  ExpectRoundTrip(scada::Variant{});
}

TEST(ConversionTest, VariantArrays) {
  ExpectRoundTrip(scada::Variant{std::vector<scada::Int32>{1, 2, 3}});
  ExpectRoundTrip(scada::Variant{std::vector<scada::String>{"a", "b"}});
  ExpectRoundTrip(scada::Variant{
      std::vector<scada::NodeId>{scada::NodeId{1u}, scada::NodeId{2u}}});
}

TEST(ConversionTest, ReadValueId) {
  ExpectRoundTrip(
      scada::ReadValueId{.node_id = scada::NodeId{9u, 1},
                         .attribute_id = scada::AttributeId::DisplayName});
}

TEST(ConversionTest, BrowseDescriptionAndResult) {
  ExpectRoundTrip(
      scada::BrowseDescription{.node_id = scada::NodeId{84u},
                               .direction = scada::BrowseDirection::Forward,
                               .reference_type_id = scada::NodeId{33u},
                               .include_subtypes = true});

  scada::BrowseResult br;
  br.status_code = scada::StatusCode::Good;
  br.references.push_back(
      scada::ReferenceDescription{.reference_type_id = scada::NodeId{35u},
                                  .forward = true,
                                  .node_id = scada::NodeId{2253u},
                                  .node_class = scada::NodeClass::Object});
  ExpectRoundTrip(br);
}

TEST(ConversionTest, DataValue) {
  scada::DataValue dv;
  dv.value = scada::Variant{scada::Double{42.0}};
  dv.status_code = scada::StatusCode::Good;
  dv.source_timestamp = base::Time::FromInternalValue(1000);
  dv.server_timestamp = base::Time::FromInternalValue(2000);
  ExpectRoundTrip(dv);
}

// The wire MonitoringFilter has no typed event/aggregate slot, so the bridge
// serializes core event/aggregate filters to a self-describing json blob and
// back. These round-trips guard that path (the typed filter must survive).
TEST(ConversionTest, MonitoringFilterEventRoundTrip) {
  scada::EventFilter ef;
  ef.types = scada::EventFilter::ACKED | scada::EventFilter::UNACKED;
  ef.of_type = {scada::NodeId{2041u},
                scada::NodeId{scada::String{"AlarmType"}, 1}};
  ef.child_of = {scada::NodeId{85u}};
  scada::MonitoringParameters params;
  params.filter = ef;

  const scada::MonitoringFilter round = ToScada(ToOpcua(params)).filter;
  ASSERT_TRUE(std::holds_alternative<scada::EventFilter>(round));
  EXPECT_EQ(std::get<scada::EventFilter>(round), ef);
}

TEST(ConversionTest, MonitoringFilterAggregateRoundTrip) {
  scada::AggregateFilter af;
  af.start_time = base::Time::FromInternalValue(123456789);
  af.interval = base::TimeDelta::FromMilliseconds(500);
  af.aggregate_type = scada::NodeId{2342u};
  scada::MonitoringParameters params;
  params.filter = af;

  const scada::MonitoringFilter round = ToScada(ToOpcua(params)).filter;
  ASSERT_TRUE(std::holds_alternative<scada::AggregateFilter>(round));
  EXPECT_EQ(std::get<scada::AggregateFilter>(round), af);
}

}  // namespace
}  // namespace scada::opcua_bridge
