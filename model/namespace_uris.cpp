#include "model/namespace_uris.h"

#include "model/namespaces.h"

namespace scada::model {

namespace {

// Canonical namespace indexes that have no NamespaceIndexes:: constant (the
// model generator emits constants only for names referenced from C++). The
// values are frozen — see common/model/nodesets/namespaces.csv and the
// ModelFrozenIds guard; the regression test cross-checks them against the
// model registry via GetNamespaceName().
constexpr scada::NamespaceIndex kHistoryNamespace = 8;
constexpr scada::NamespaceIndex kDevicesNamespace = 13;
constexpr scada::NamespaceIndex kFilesystemNamespace = 19;
constexpr scada::NamespaceIndex kDataItemsNamespace = 20;
constexpr scada::NamespaceIndex kSecurityNamespace = 21;
constexpr scada::NamespaceIndex kIec61850ServerNamespace = 22;

}  // namespace

std::vector<std::string> GetCanonicalNamespaceUris() {
  std::vector<std::string> uris(NamespaceIndexes::END);
  uris[0] = std::string{kOpcUaNamespaceUri};
  uris[NamespaceIndexes::TS] =
      "http://telecontrol.ru/opcua/data_items/DiscreteItemType";
  uris[NamespaceIndexes::TIT] =
      "http://telecontrol.ru/opcua/data_items/AnalogItemType";
  uris[NamespaceIndexes::MODBUS_DEVICES] =
      "http://telecontrol.ru/opcua/devices/ModbusDeviceType";
  uris[NamespaceIndexes::GROUP] =
      "http://telecontrol.ru/opcua/data_items/DataGroupType";
  uris[NamespaceIndexes::USER] =
      "http://telecontrol.ru/opcua/security/UserType";
  uris[NamespaceIndexes::HISTORICAL_DB] =
      "http://telecontrol.ru/opcua/history/HistoricalDatabaseType";
  uris[NamespaceIndexes::SIM_ITEM] =
      "http://telecontrol.ru/opcua/history/data_items/SimulationSignalType";
  uris[NamespaceIndexes::IEC60870_LINK] =
      "http://telecontrol.ru/opcua/devices/Iec60870LinkType";
  uris[NamespaceIndexes::IEC60870_DEVICE] =
      "http://telecontrol.ru/opcua/devices/Iec60870DeviceType";
  uris[NamespaceIndexes::MODBUS_PORTS] =
      "http://telecontrol.ru/opcua/devices/ModbusLinkType";
  uris[NamespaceIndexes::TS_FORMAT] =
      "http://telecontrol.ru/opcua/data_items/TsFormatType";
  uris[NamespaceIndexes::IEC60870_TRANSMISSION_ITEM] =
      "http://telecontrol.ru/opcua/devices/Iec60870TransmissionItemType";
  uris[NamespaceIndexes::MODBUS_TRANSMISSION_ITEM] =
      "http://telecontrol.ru/opcua/devices/ModbusTransmissionItemType";
  uris[NamespaceIndexes::IEC61850_TRANSMISSION_ITEM] =
      "http://telecontrol.ru/opcua/devices/Iec61850TransmissionItemType";
  uris[NamespaceIndexes::IEC61850_DEVICE] =
      "http://telecontrol.ru/opcua/devices/Iec61850DeviceType";
  uris[NamespaceIndexes::IEC61850_RCB] =
      "http://telecontrol.ru/opcua/devices/Iec61850RcbType";
  uris[NamespaceIndexes::SCADA] = "http://telecontrol.ru/opcua/scada";
  // Required for aggregating the filesystem tier: file/directory instance
  // nodes live in this namespace, and ProxyNamespaceTable::GetOrAdd merges
  // equal URIs — with the historical empty URI every empty-URI namespace
  // collapsed onto the first empty index, mis-remapping downstream file nodes.
  // A real URI makes same-model tiers merge it identity and lets a
  // proxy-config routing claim name it.
  uris[NamespaceIndexes::FILESYSTEM_FILE] =
      "http://telecontrol.ru/opcua/filesystem/FileType";
  uris[NamespaceIndexes::ROLE] =
      "http://telecontrol.ru/opcua/security/RoleType";
  uris[NamespaceIndexes::ROLE_IDENTITY] =
      "http://telecontrol.ru/opcua/security/IdentityMappingRuleType";
  uris[NamespaceIndexes::CONFIGURATION] =
      "http://telecontrol.ru/opcua/security/ConfigurationType";
  // Every canonical index must carry a real, unique URI. OPC UA Part 3 §8.2.3
  // defines NamespaceArray entries as namespace URIs (index 0 = OPC UA, the
  // rest identified by URI), and ProxyNamespaceTable::GetOrAdd merges by URI
  // equality — with more than one empty entry, an Aggregating Server merging a
  // downstream's NamespaceArray would collapse ALL empty-URI namespaces onto
  // the first empty index and silently mis-remap every NodeId in them.
  // https://reference.opcfoundation.org/Core/Part3/v105/docs/8.2.3
  uris[kHistoryNamespace] = "http://telecontrol.ru/opcua/history";
  uris[kDevicesNamespace] = "http://telecontrol.ru/opcua/devices";
  uris[NamespaceIndexes::SERVER_PARAMS] =
      "http://telecontrol.ru/opcua/server_params";
  uris[kFilesystemNamespace] = "http://telecontrol.ru/opcua/filesystem";
  uris[kDataItemsNamespace] = "http://telecontrol.ru/opcua/data_items";
  uris[kSecurityNamespace] = "http://telecontrol.ru/opcua/security";
  uris[kIec61850ServerNamespace] =
      "http://telecontrol.ru/opcua/iec61850_server";
  uris[NamespaceIndexes::ALIAS] = "http://telecontrol.ru/opcua/alias";
  uris[NamespaceIndexes::OPC] = "http://telecontrol.ru/opcua/opc";
  uris[NamespaceIndexes::VIDICON] = "http://telecontrol.ru/opcua/vidicon";
  uris[NamespaceIndexes::VIDICON_FILE] =
      "http://telecontrol.ru/opcua/vidicon/FileType";
  uris[NamespaceIndexes::HISTORICAL_CONFIG] =
      "http://telecontrol.ru/opcua/history/HistoricalDataConfigurationType";
  return uris;
}

}  // namespace scada::model
