#include <gmock/gmock.h>

#include "core/namespaces.h"
#include "core/node_id.h"

namespace scada {

TEST(NodeId, FromString) {
  EXPECT_EQ(NodeId(53, NamespaceIndexes::TS), NodeId::FromString("TS.53"));
  EXPECT_EQ(NodeId(15, NamespaceIndexes::IEC_DEV), NodeId::FromString("IEC_DEV.15"));
  EXPECT_EQ(NodeId(12, NamespaceIndexes::IEC_DEV), NodeId::FromString("T11.12"));
  EXPECT_EQ(NodeId("MODBUS_PORTS.32!Online", 0), NodeId::FromString("MODBUS_PORTS.32!Online"));
  EXPECT_EQ(NodeId("MODBUS_PORTS.32!BIT:4", 0), NodeId::FromString("MODBUS_PORTS.32!BIT:4"));
}

TEST(NodeId, ToString) {
  EXPECT_EQ(NodeId(53, NamespaceIndexes::TS).ToString(), "TS.53");
  EXPECT_EQ(NodeId(15, NamespaceIndexes::IEC_DEV).ToString(), "IEC_DEV.15");
  EXPECT_EQ(NodeId("MODBUS_PORTS.32!Online", 0).ToString(), "MODBUS_PORTS.32!Online");
  EXPECT_EQ(NodeId("MODBUS_PORTS.32!BIT:4", 0).ToString(), "MODBUS_PORTS.32!BIT:4");
}

TEST(NodeId, Comparison) {
  EXPECT_LT(NodeId::FromString("HISTORICAL_DB.1!WriteEventDuration"),
            NodeId::FromString("HISTORICAL_DB.1!WriteValueDuration"));
  
  EXPECT_LT(NodeId::FromString("HISTORICAL_DB.1!WriteEventDuration"),
            NodeId::FromString("HISTORICAL_DB.3!WriteEventDuration"));

  EXPECT_EQ(NodeId::FromString("HISTORICAL_DB.1!WriteEventDuration"),
            NodeId::FromString("HISTORICAL_DB.1!WriteEventDuration"));
}

} // namespace scada
