#include "opcua_bridge/conversion.h"
#include "opcua_bridge/service_conversion.h"

#include <gtest/gtest.h>

namespace opcua_bridge {
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

TEST(ConversionTest, DateTime) {
  ExpectRoundTrip(base::Time::FromInternalValue(123456789));
  ExpectRoundTrip(base::Time{});
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
  ExpectRoundTrip(
      scada::Variant{std::vector<scada::NodeId>{scada::NodeId{1u}, scada::NodeId{2u}}});
}

TEST(ConversionTest, ReadValueId) {
  ExpectRoundTrip(
      scada::ReadValueId{.node_id = scada::NodeId{9u, 1},
                         .attribute_id = scada::AttributeId::DisplayName});
}

TEST(ConversionTest, BrowseDescriptionAndResult) {
  ExpectRoundTrip(scada::BrowseDescription{
      .node_id = scada::NodeId{84u},
      .direction = scada::BrowseDirection::Forward,
      .reference_type_id = scada::NodeId{33u},
      .include_subtypes = true});

  scada::BrowseResult br;
  br.status_code = scada::StatusCode::Good;
  br.references.push_back(scada::ReferenceDescription{
      .reference_type_id = scada::NodeId{35u},
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

}  // namespace
}  // namespace opcua_bridge
