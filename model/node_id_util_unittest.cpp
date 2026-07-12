#include "model/node_id_util.h"

#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "scada/node_id.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(NodeIdUtil, NodeIdFromScadaString) {
  EXPECT_EQ(scada::NodeId(53, NamespaceIndexes::TS),
            NodeIdFromScadaString("TS.53"));
  EXPECT_EQ(scada::NodeId(15, NamespaceIndexes::IEC60870_DEVICE),
            NodeIdFromScadaString("IEC_DEV.15"));
  EXPECT_EQ(
      scada::NodeId(12, NamespaceIndexes::IEC60870_DEVICE),
      NodeIdFromScadaString("T11.12"));  // NamespaceIndexes::IEC60870_DEVICE
  EXPECT_EQ(scada::NodeId("32!Online", NamespaceIndexes::MODBUS_PORTS),
            NodeIdFromScadaString("MODBUS_PORTS.32!Online"));
  EXPECT_EQ(scada::NodeId("32!BIT:4", NamespaceIndexes::MODBUS_PORTS),
            NodeIdFromScadaString("MODBUS_PORTS.32!BIT:4"));
  EXPECT_EQ(data_items::id::Statistics_ServerCPUUsage,
            NodeIdFromScadaString("Server!PCPU"));
  EXPECT_EQ(data_items::id::Statistics_TotalMemoryUsage,
            NodeIdFromScadaString("Server!Mem"));
  EXPECT_EQ(scada::NodeId{}, NodeIdFromScadaString("Alias"));
  EXPECT_EQ(scada::NodeId{}, NodeIdFromScadaString("alias_with_underscores"));
}

TEST(NodeIdUtil, NodeIdToScadaString) {
  EXPECT_EQ(NodeIdToScadaString(scada::NodeId(53, NamespaceIndexes::TS)),
            "TS.53");
  EXPECT_EQ(
      NodeIdToScadaString(scada::NodeId(15, NamespaceIndexes::IEC60870_DEVICE)),
      "IEC_DEV.15");
  EXPECT_EQ(NodeIdToScadaString(
                scada::NodeId("32!Online", NamespaceIndexes::MODBUS_PORTS)),
            "MODBUS_PORTS.32!Online");
  EXPECT_EQ(NodeIdToScadaString(
                scada::NodeId("32!BIT:4", NamespaceIndexes::MODBUS_PORTS)),
            "MODBUS_PORTS.32!BIT:4");
  EXPECT_EQ(NodeIdToScadaString(data_items::id::Statistics_ServerCPUUsage),
            "Server!PCPU");
  EXPECT_EQ(NodeIdToScadaString(data_items::id::Statistics_TotalMemoryUsage),
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
  scada::NodeId kNodeId{"123!Online", NamespaceIndexes::MODBUS_DEVICES};
  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_TRUE(IsNestedNodeId(kNodeId, parent_id, nested_name));
  EXPECT_EQ(scada::NodeId(123, NamespaceIndexes::MODBUS_DEVICES), parent_id);
  EXPECT_EQ("Online", std::string{nested_name});
}

TEST(NodeIdUtil, IsNestedNodeId_Empty) {
  scada::NodeId kNodeId{"123!", NamespaceIndexes::MODBUS_DEVICES};
  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_FALSE(IsNestedNodeId(kNodeId, parent_id, nested_name));
  EXPECT_THAT(nested_name, IsEmpty());
}

TEST(NodeIdUtil, IsNestedNodeId_LevelTwo) {
  scada::NodeId kNodeId{"1!Model!ENIP2BAYCTRL",
                        NamespaceIndexes::IEC61850_DEVICE};
  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_TRUE(IsNestedNodeId(kNodeId, parent_id, nested_name));
  EXPECT_EQ(scada::NodeId("1!Model", NamespaceIndexes::IEC61850_DEVICE),
            parent_id);
  EXPECT_EQ("ENIP2BAYCTRL", std::string{nested_name});
}

TEST(NodeIdUtil, IsNestedNodeId_StringIdParent) {
  scada::NodeId kNodeId{"Test.OpcServerProgId\\BranchB.LeafBA!Output",
                        NamespaceIndexes::OPC};
  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_TRUE(IsNestedNodeId(kNodeId, parent_id, nested_name));
  EXPECT_EQ(scada::NodeId("Test.OpcServerProgId\\BranchB.LeafBA",
                          NamespaceIndexes::OPC),
            parent_id);
  EXPECT_EQ("Output", std::string{nested_name});
}

TEST(NodeIdUtil, MakeNestedNodeId) {
  const scada::NodeId kParentId{456, NamespaceIndexes::MODBUS_DEVICES};
  EXPECT_EQ(scada::NodeId("456!Active", NamespaceIndexes::MODBUS_DEVICES),
            MakeNestedNodeId(kParentId, "Active"));
}

TEST(NodeIdUtil, NestedNodeId_NestedNamespaceDiffers) {
  const scada::NodeId kParentId{456, NamespaceIndexes::MODBUS_DEVICES};
  const scada::NodeId kNestedId{"3:456!ConnectCount", NamespaceIndexes::SCADA};
  const std::string_view kNestedName = "ConnectCount";
  EXPECT_EQ(kNestedId,
            MakeNestedNodeId(kParentId, kNestedName, NamespaceIndexes::SCADA));

  scada::NodeId parent_id;
  std::string_view nested_name;
  ASSERT_TRUE(IsNestedNodeId(kNestedId, parent_id, nested_name));
  EXPECT_EQ(kParentId, parent_id);
  EXPECT_EQ(kNestedName, nested_name);
}
