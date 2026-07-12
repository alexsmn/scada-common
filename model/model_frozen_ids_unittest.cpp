// Regression guard for the model's frozen identifiers.
//
// The numeric node ids and the namespace short names are persisted in user
// configuration databases (config tables are keyed by these ids and named after
// the namespace short names). They must therefore never change: a
// `symbolicName` in a nodeset may rename the C++ handle, but the underlying id
// is frozen. This test pins those values so an accidental renumbering (e.g.
// editing a nodeset id or reordering the namespace manifest) fails the build
// instead of silently breaking every existing database.
//
// When you intentionally ADD a node, add its constant here too; you should
// never need to CHANGE an existing expectation.

#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "model/history_node_ids.h"
#include "model/namespaces.h"
#include "model/opc_node_ids.h"
#include "model/scada_node_ids.h"
#include "model/security_node_ids.h"
#include "model/security_node_ids_util.h"

#include <string_view>

#include <gtest/gtest.h>

namespace scada {
namespace {

// The namespace short names double as configuration-DB table names and must
// stay byte-for-byte identical in this exact index order.
TEST(ModelFrozenIds, NamespaceShortNames) {
  static constexpr std::string_view kExpected[] = {"NS0",
                                                   "TS",
                                                   "TIT",
                                                   "MODBUS_DEVICES",
                                                   "GROUP",
                                                   "USER",
                                                   "HISTORICAL_DB",
                                                   "SCADA",
                                                   "HISTORY",
                                                   "SIM_ITEM",
                                                   "IEC_LINK",
                                                   "IEC_DEV",
                                                   "MODBUS_PORTS",
                                                   "DEVICES",
                                                   "TS_PARAMS",
                                                   "SERVER_PARAMS",
                                                   "IEC_TRANSMIT",
                                                   "IEC61850_DEV",
                                                   "IEC61850_RCB",
                                                   "FILESYSTEM",
                                                   "DATA_ITEMS",
                                                   "SECURITY",
                                                   "IEC61850_SERVER",
                                                   "ALIAS",
                                                   "FILESYSTEM_FILE",
                                                   "OPC",
                                                   "MODBUS_TRANSMIT",
                                                   "IEC61850_TRANSMIT",
                                                   "VIDICON",
                                                   "VIDICON_FILE",
                                                   "ROLE",
                                                   "ROLE_IDENTITY",
                                                   "CONFIGURATION",
                                                   "HISTORICAL_CONFIG"};
  ASSERT_EQ(std::size(kExpected), NamespaceIndexes::END);
  for (NamespaceIndex i = 0; i != NamespaceIndexes::END; ++i)
    EXPECT_EQ(GetNamespaceName(i), kExpected[i]) << "namespace index " << i;
}

// Namespace indices are also persisted (they are the numeric namespace of
// stored NodeIds). Pin the ones exposed as C++ constants.
TEST(ModelFrozenIds, NamespaceIndices) {
  EXPECT_EQ(NamespaceIndexes::NS0, 0);
  EXPECT_EQ(NamespaceIndexes::USER, 5);
  EXPECT_EQ(NamespaceIndexes::HISTORICAL_DB, 6);
  EXPECT_EQ(NamespaceIndexes::SCADA, 7);
  EXPECT_EQ(NamespaceIndexes::IEC60870_LINK, 10);
  EXPECT_EQ(NamespaceIndexes::OPC, 25);
  EXPECT_EQ(NamespaceIndexes::ROLE, 30);
  EXPECT_EQ(NamespaceIndexes::CONFIGURATION, 32);
  EXPECT_EQ(NamespaceIndexes::HISTORICAL_CONFIG, 33);
  EXPECT_EQ(NamespaceIndexes::END, 34);
}

// Representative node-id values spanning every domain. These are stored in
// configuration databases and must not move.
TEST(ModelFrozenIds, NodeIdValues) {
  // scada:: core type system.
  EXPECT_EQ(id::PropertyCategories.numeric_id(), 310u);
  EXPECT_EQ(id::MetricType.numeric_id(), 336u);
  EXPECT_EQ(id::NextId.numeric_id(), 352u);

  // devices::
  EXPECT_EQ(devices::id::DeviceType.numeric_id(), 115u);
  EXPECT_EQ(devices::id::LinkType.numeric_id(), 116u);
  EXPECT_EQ(devices::id::DeviceType_Online.numeric_id(), 248u);
  EXPECT_EQ(devices::id::LinkType_Transport.numeric_id(), 142u);
  EXPECT_EQ(devices::id::ModbusDeviceType.numeric_id(), 118u);
  EXPECT_EQ(devices::id::Iec60870DeviceType.numeric_id(), 331u);
  EXPECT_EQ(devices::id::DeviceType_Interrogate.numeric_id(),
            133u);  // runtime method
  EXPECT_EQ(devices::id::TransmissionItemType_SourceAddress.numeric_id(), 225u);
  EXPECT_EQ(devices::id::HasTransmissionItem.numeric_id(), 339u);
  EXPECT_EQ(devices::numeric_id::Iec60870DeviceType_InterrogationPeriodGroup1,
            200u);

  // data_items::
  EXPECT_EQ(data_items::id::DataItemType.numeric_id(), 239u);
  EXPECT_EQ(data_items::id::Statistics_TotalCPUUsage.numeric_id(),
            228u);  // runtime metric
  EXPECT_EQ(data_items::numeric_id::TsFormats, 27u);
  EXPECT_EQ(data_items::numeric_id::SimulationSignals, 28u);

  // history::
  EXPECT_EQ(history::numeric_id::HistoricalDatabases, 12u);
  EXPECT_EQ(history::id::HistoricalDatabaseType_ItemCount.numeric_id(), 244u);
  // The historian's external-node historization entries (ADR 0002 §2c): our
  // subtype of the standard HistoricalDataConfigurationType (NS0 i=2318, OPC
  // UA Part 11 §5.2.2), subject-linked via HasHistoricalConfiguration (NS0
  // i=56) — the NS0 ids are core constants, not model-generated.
  EXPECT_EQ(history::id::HistoricalDataConfigurationType.numeric_id(), 368u);
  EXPECT_EQ(history::id::HistoricalDataConfigurations.numeric_id(), 369u);

  // security::
  EXPECT_EQ(security::numeric_id::Users, 29u);
  EXPECT_EQ(security::numeric_id::Configurations, 362u);
  EXPECT_EQ(security::id::UserType_ProfileRevision.numeric_id(), 347u);
  EXPECT_EQ(security::id::Configurations_DeployConfiguration.numeric_id(),
            365u);
  // RoleSet is the NS0 standard node, kept hand-written.
  EXPECT_EQ(security::id::RoleSet.namespace_index(), NamespaceIndexes::NS0);

  // filesystem:: and opc::
  EXPECT_EQ(filesystem::id::FileSystem.numeric_id(), 304u);
  EXPECT_EQ(opc::id::OPC.numeric_id(), 329u);
}

}  // namespace
}  // namespace scada
