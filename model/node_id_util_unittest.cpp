#include "model/node_id_util.h"

#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "scada/node_id.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(NodeIdUtil, NodeIdFromScadaString) {
  EXPECT_EQ(scada::NodeId(53, scada::NamespaceIndexes::TS),
            NodeIdFromScadaString("TS.53"));
  EXPECT_EQ(scada::NodeId(15, scada::NamespaceIndexes::IEC60870_DEVICE),
            NodeIdFromScadaString("IEC_DEV.15"));
  EXPECT_EQ(
      scada::NodeId(12, scada::NamespaceIndexes::IEC60870_DEVICE),
      NodeIdFromScadaString("T11.12"));  // NamespaceIndexes::IEC60870_DEVICE
  EXPECT_EQ(scada::NodeId("32!Online", scada::NamespaceIndexes::MODBUS_PORTS),
            NodeIdFromScadaString("MODBUS_PORTS.32!Online"));
  EXPECT_EQ(scada::NodeId("32!BIT:4", scada::NamespaceIndexes::MODBUS_PORTS),
            NodeIdFromScadaString("MODBUS_PORTS.32!BIT:4"));
  EXPECT_EQ(scada::data_items::id::Statistics_ServerCPUUsage,
            NodeIdFromScadaString("Server!PCPU"));
  EXPECT_EQ(scada::data_items::id::Statistics_TotalMemoryUsage,
            NodeIdFromScadaString("Server!Mem"));
  EXPECT_EQ(scada::NodeId{}, NodeIdFromScadaString("Alias"));
  EXPECT_EQ(scada::NodeId{}, NodeIdFromScadaString("alias_with_underscores"));
}

TEST(NodeIdUtil, NodeIdToScadaString) {
  EXPECT_EQ(NodeIdToScadaString(scada::NodeId(53, scada::NamespaceIndexes::TS)),
            "TS.53");
  EXPECT_EQ(NodeIdToScadaString(
                scada::NodeId(15, scada::NamespaceIndexes::IEC60870_DEVICE)),
            "IEC_DEV.15");
  EXPECT_EQ(NodeIdToScadaString(scada::NodeId(
                "32!Online", scada::NamespaceIndexes::MODBUS_PORTS)),
            "MODBUS_PORTS.32!Online");
  EXPECT_EQ(NodeIdToScadaString(scada::NodeId(
                "32!BIT:4", scada::NamespaceIndexes::MODBUS_PORTS)),
            "MODBUS_PORTS.32!BIT:4");
  EXPECT_EQ(
      NodeIdToScadaString(scada::data_items::id::Statistics_ServerCPUUsage),
      "Server!PCPU");
  EXPECT_EQ(
      NodeIdToScadaString(scada::data_items::id::Statistics_TotalMemoryUsage),
      "Server!Mem");
}

TEST(NodeIdUtil, Comparison) {
  EXPECT_LT(NodeIdFromScadaString("HISTORICAL_DB.1!WriteEventDuration"),
            NodeIdFromScadaString("HISTORICAL_DB.1!WriteValueDuration"));

  EXPECT_LT(NodeIdFromScadaString("HISTORICAL_DB.1!WriteEventDuration"),
            NodeIdFromScadaString("HISTORICAL_DB.3!WriteEventDuration"));

  EXPECT_EQ(NodeIdFromScadaString("HISTORICAL_DB.1!WriteEventDuration"),
            NodeIdFromScadaString("HISTORICAL_DB.1!WriteEventDuration"));
}

TEST(NodeIdUtil, IsNestedNodeId) {
  scada::NodeId kNodeId{"123!Online", scada::NamespaceIndexes::MODBUS_DEVICES};
  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_TRUE(IsNestedNodeId(kNodeId, parent_id, nested_name));
  EXPECT_EQ(scada::NodeId(123, scada::NamespaceIndexes::MODBUS_DEVICES),
            parent_id);
  EXPECT_EQ("Online", std::string{nested_name});
}

TEST(NodeIdUtil, IsNestedNodeId_Empty) {
  scada::NodeId kNodeId{"123!", scada::NamespaceIndexes::MODBUS_DEVICES};
  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_FALSE(IsNestedNodeId(kNodeId, parent_id, nested_name));
  EXPECT_THAT(nested_name, IsEmpty());
}

TEST(NodeIdUtil, IsNestedNodeId_LevelTwo) {
  scada::NodeId kNodeId{"1!Model!ENIP2BAYCTRL",
                        scada::NamespaceIndexes::IEC61850_DEVICE};
  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_TRUE(IsNestedNodeId(kNodeId, parent_id, nested_name));
  EXPECT_EQ(scada::NodeId("1!Model", scada::NamespaceIndexes::IEC61850_DEVICE),
            parent_id);
  EXPECT_EQ("ENIP2BAYCTRL", std::string{nested_name});
}

TEST(NodeIdUtil, IsNestedNodeId_StringIdParent) {
  scada::NodeId kNodeId{"Test.OpcServerProgId\\BranchB.LeafBA!Output",
                        scada::NamespaceIndexes::OPC};
  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_TRUE(IsNestedNodeId(kNodeId, parent_id, nested_name));
  EXPECT_EQ(scada::NodeId("Test.OpcServerProgId\\BranchB.LeafBA",
                          scada::NamespaceIndexes::OPC),
            parent_id);
  EXPECT_EQ("Output", std::string{nested_name});
}

TEST(NodeIdUtil, MakeNestedNodeId) {
  const scada::NodeId kParentId{456, scada::NamespaceIndexes::MODBUS_DEVICES};
  EXPECT_EQ(
      scada::NodeId("456!Active", scada::NamespaceIndexes::MODBUS_DEVICES),
      MakeNestedNodeId(kParentId, "Active"));
}

TEST(NodeIdUtil, NestedNodeId_NestedNamespaceDiffers) {
  const scada::NodeId kParentId{456, scada::NamespaceIndexes::MODBUS_DEVICES};
  const scada::NodeId kNestedId{"3:456!ConnectCount",
                                scada::NamespaceIndexes::SCADA};
  const std::string_view kNestedName = "ConnectCount";
  EXPECT_EQ(kNestedId, MakeNestedNodeId(kParentId, kNestedName,
                                        scada::NamespaceIndexes::SCADA));

  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_TRUE(IsNestedNodeId(kNestedId, parent_id, nested_name));
  EXPECT_EQ(kParentId, parent_id);
  EXPECT_EQ(kNestedName, nested_name);
}

namespace scada {
namespace {

// The self-contained OPC UA reference form, as stored in display bindings
// (docs/product/foreign-node-references.md). A connection that gives a plant
// namespace index 200 and knows nothing else.
std::optional<NamespaceIndex> PlantIndex(std::string_view uri) {
  if (uri == "urn:plant")
    return NamespaceIndex{200};
  return std::nullopt;
}

TEST(NodeIdUtil, NodeIdStringResolvesThroughTheConnection) {
  EXPECT_EQ(
      NodeIdFromNodeIdString("nsu=urn:plant;s=Plant.Temperature", PlantIndex),
      NodeId(std::string{"Plant.Temperature"}, 200));
}

TEST(NodeIdUtil, NodeIdStringFallsBackToTheCanonicalTable) {
  // A model namespace resolves without the connection: the native backend
  // numbers it canonically and sets no lookup.
  const NodeId expected{53, NamespaceIndexes::TS};
  EXPECT_EQ(NodeIdFromNodeIdString(
                "nsu=http://telecontrol.ru/opcua/data_items/DiscreteItemType;"
                "i=53",
                PlantIndex),
            expected);
  EXPECT_EQ(NodeIdFromNodeIdString(
                "nsu=http://telecontrol.ru/opcua/data_items/DiscreteItemType;"
                "i=53",
                NamespaceIndexForUri{}),
            expected);
}

TEST(NodeIdUtil, NodeIdStringInNamespaceZero) {
  EXPECT_EQ(NodeIdFromNodeIdString("i=2258", NamespaceIndexForUri{}),
            NodeId(2258u, 0));
}

TEST(NodeIdUtil, NodeIdStringThatNamesNothingIsNull) {
  // The form, but unresolvable: an error for the caller, never an alias.
  EXPECT_EQ(NodeIdFromNodeIdString("nsu=urn:other;i=1", PlantIndex),
            std::optional<NodeId>{NodeId{}});
  // A session's own index means nothing once stored.
  EXPECT_EQ(NodeIdFromNodeIdString("ns=2;i=1", PlantIndex),
            std::optional<NodeId>{NodeId{}});
}

TEST(NodeIdUtil, OtherFormsAreNotNodeIdStrings) {
  for (std::string_view text : {"TS.53", "Alias", "MODBUS_PORTS.32!Online",
                                "Server!PCPU", "NS38.Plant"}) {
    SCOPED_TRACE(text);
    EXPECT_EQ(NodeIdFromNodeIdString(text, PlantIndex), std::nullopt);
  }
}

}  // namespace
}  // namespace scada
