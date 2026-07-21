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

#include "model/config_tables.h"
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
  EXPECT_EQ(devices::id::TransmissionItemType_Address.numeric_id(), 225u);
  // ns=1;i=221 (HasTransmissionSource) is RETIRED but stays reserved — never
  // reallocate it (transmission alignment phase 4; the source link is the
  // SourceNode property below, like i=297 Creates before it).
  EXPECT_EQ(devices::id::TransmissionItemType_SourceNode.numeric_id(), 373u);
  EXPECT_EQ(devices::id::HasTransmissionItem.numeric_id(), 339u);
  // Transmission-item OptionalPlaceholder InstanceDeclarations on the
  // protocol device types (OPC UA Part 3 §6.4.4.4.4).
  EXPECT_EQ(
      devices::id::Iec60870DeviceType_TransmissionItemPlaceholder.numeric_id(),
      370u);
  EXPECT_EQ(
      devices::id::ModbusDeviceType_TransmissionItemPlaceholder.numeric_id(),
      371u);
  EXPECT_EQ(
      devices::id::Iec61850DeviceType_TransmissionItemPlaceholder.numeric_id(),
      372u);
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

// The generated config-table registry (namespaces.csv row_type/config_group
// columns) binds each device config table's row type to the namespace whose
// index every stored row NodeId carries. Both sides of each pair are persisted
// in configuration databases, so the association is frozen too.
TEST(ModelFrozenIds, ConfigTableRegistry) {
  ASSERT_EQ(std::size(model::kConfigTables), 20u);

  const auto expect_entry =
      [](const model::ConfigTableEntry& entry, const NodeId& type_definition_id,
         NamespaceIndex namespace_index, std::string_view config_group,
         std::string_view group_kind) {
        EXPECT_EQ(entry.type_definition_id, type_definition_id);
        EXPECT_EQ(entry.namespace_index, namespace_index);
        EXPECT_EQ(entry.config_group, config_group);
        EXPECT_EQ(entry.group_kind, group_kind);
      };

  const auto expect_module = [](const model::ModuleNamespaceEntry& entry,
                                NamespaceIndex namespace_index,
                                std::string_view config_group) {
    EXPECT_EQ(entry.namespace_index, namespace_index);
    EXPECT_EQ(entry.config_group, config_group);
  };

  const auto tables = model::GetConfigTables();
  expect_entry(tables[0], data_items::id::DiscreteItemType,
               NamespaceIndexes::TS, "data_items", "dedicated");
  expect_entry(tables[1], data_items::id::AnalogItemType, NamespaceIndexes::TIT,
               "data_items", "dedicated");
  expect_entry(tables[2], devices::id::ModbusDeviceType,
               NamespaceIndexes::MODBUS_DEVICES, "modbus", "device");
  expect_entry(tables[3], data_items::id::DataGroupType,
               NamespaceIndexes::GROUP, "data_items", "dedicated");
  expect_entry(tables[4], security::id::UserType, NamespaceIndexes::USER,
               "security", "dedicated");
  expect_entry(tables[5], history::id::HistoricalDatabaseType,
               NamespaceIndexes::HISTORICAL_DB, "history", "dedicated");
  expect_entry(tables[6], data_items::id::SimulationSignalType,
               NamespaceIndexes::SIM_ITEM, "data_items", "dedicated");
  expect_entry(tables[7], devices::id::Iec60870LinkType,
               NamespaceIndexes::IEC60870_LINK, "iec60870", "device");
  expect_entry(tables[8], devices::id::Iec60870DeviceType,
               NamespaceIndexes::IEC60870_DEVICE, "iec60870", "device");
  expect_entry(tables[9], devices::id::ModbusLinkType,
               NamespaceIndexes::MODBUS_PORTS, "modbus", "device");
  expect_entry(tables[10], data_items::id::TsFormatType,
               NamespaceIndexes::TS_FORMAT, "data_items", "dedicated");
  expect_entry(tables[11], devices::id::Iec60870TransmissionItemType,
               NamespaceIndexes::IEC60870_TRANSMISSION_ITEM, "iec60870",
               "device");
  expect_entry(tables[12], devices::id::Iec61850DeviceType,
               NamespaceIndexes::IEC61850_DEVICE, "iec61850", "device");
  expect_entry(tables[13], devices::id::Iec61850RcbType,
               NamespaceIndexes::IEC61850_RCB, "iec61850", "device");
  expect_entry(tables[14], devices::id::ModbusTransmissionItemType,
               NamespaceIndexes::MODBUS_TRANSMISSION_ITEM, "modbus", "device");
  expect_entry(tables[15], devices::id::Iec61850TransmissionItemType,
               NamespaceIndexes::IEC61850_TRANSMISSION_ITEM, "iec61850",
               "device");
  expect_entry(tables[16], security::id::RoleType, NamespaceIndexes::ROLE,
               "security", "dedicated");
  expect_entry(tables[17], security::id::IdentityMappingRuleType,
               NamespaceIndexes::ROLE_IDENTITY, "security", "dedicated");
  expect_entry(tables[18], security::id::ConfigurationType,
               NamespaceIndexes::CONFIGURATION, "security", "dedicated");
  expect_entry(tables[19], history::id::HistoricalDataConfigurationType,
               NamespaceIndexes::HISTORICAL_CONFIG, "history", "dedicated");

  ASSERT_EQ(std::size(model::kConfigTableGroups), 6u);
  EXPECT_EQ(model::kConfigTableGroups[0], "data_items");
  EXPECT_EQ(model::kConfigTableGroups[1], "modbus");
  EXPECT_EQ(model::kConfigTableGroups[2], "security");
  EXPECT_EQ(model::kConfigTableGroups[3], "history");
  EXPECT_EQ(model::kConfigTableGroups[4], "iec60870");
  EXPECT_EQ(model::kConfigTableGroups[5], "iec61850");

  ASSERT_EQ(std::size(model::kDeviceConfigTableGroups), 3u);
  EXPECT_EQ(model::kDeviceConfigTableGroups[0], "modbus");
  EXPECT_EQ(model::kDeviceConfigTableGroups[1], "iec60870");
  EXPECT_EQ(model::kDeviceConfigTableGroups[2], "iec61850");

  // Module namespaces (ADR 0003): tier-exclusive namespaces with no config
  // table. Frozen indexes, since they are persisted and drive tier routing.
  ASSERT_EQ(std::size(model::kModuleNamespaces), 5u);
  // FILESYSTEM (19) has no NamespaceIndexes:: constant (no C++ references it by
  // name); its index is frozen in namespaces.csv.
  expect_module(model::kModuleNamespaces[0], 19, "filesystem");
  expect_module(model::kModuleNamespaces[1], NamespaceIndexes::FILESYSTEM_FILE,
                "filesystem");
  expect_module(model::kModuleNamespaces[2], NamespaceIndexes::OPC, "opc");
  expect_module(model::kModuleNamespaces[3], NamespaceIndexes::VIDICON,
                "vidicon");
  expect_module(model::kModuleNamespaces[4], NamespaceIndexes::VIDICON_FILE,
                "vidicon");

  ASSERT_EQ(std::size(model::kModuleNamespaceGroups), 3u);
  EXPECT_EQ(model::kModuleNamespaceGroups[0], "filesystem");
  EXPECT_EQ(model::kModuleNamespaceGroups[1], "opc");
  EXPECT_EQ(model::kModuleNamespaceGroups[2], "vidicon");
}

}  // namespace
}  // namespace scada
