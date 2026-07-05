#pragma once

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/generic_node_factory.h"
#include "address_space/standard_address_space.h"
#include "address_space/type_definition.h"
#include "base/check.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/history_node_ids.h"
#include "model/namespaces.h"
#include "model/scada_node_ids.h"
#include "scada/node_class.h"
#include "scada/standard_node_ids.h"

#include <vector>

// Code-defined SCADA test type system.
//
// Replaces the XML-loaded `AddressSpaceImpl3` for tests. It does NOT parse any
// nodeset XML; instead it builds, in C++, the subset of the `data_items` type
// system that common (and client) tests rely on (type definitions, their
// property declarations, browse/display names) on top of the standard OPC UA
// tree provided by `StandardAddressSpace`.
//
// Faithfulness matters: the display names below mirror
// `model/nodesets/data_items.xml` because tests assert on them (e.g. export
// tests check the type display name "Объект ТИТ" and the property declaration
// display name "Инверсия"). Keep them in sync with the nodeset when the type
// system changes.

namespace scada_test {

// Adds the `data_items` test type definitions and their property declarations
// to `address_space`. The standard OPC UA nodes they reference (FolderType,
// BaseVariableType, PropertyType, HasProperty, ...) must already be present,
// e.g. via a live `StandardAddressSpace`.
inline void AddScadaDataItemsTestTypes(AddressSpaceImpl& address_space) {
  GenericNodeFactory factory{address_space};

  namespace di = data_items::id;

  std::vector<scada::NodeState> nodes;

  // The "Все объекты" folder that owns exported data-item instances.
  nodes.push_back(scada::NodeState{
      .node_id = di::DataItems,
      .node_class = scada::NodeClass::Object,
      .type_definition_id = {scada::id::FolderType, NamespaceIndexes::NS0},
      .parent_id = {scada::id::ObjectsFolder, NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::Organizes, NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("DataItems")
                        .set_display_name(u"Все объекты")});

  // Base data-item type.
  nodes.push_back(scada::NodeState{
      .node_id = di::DataItemType,
      .node_class = scada::NodeClass::VariableType,
      .parent_id = {scada::id::BaseVariableType, NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{}
              .set_browse_name("DataItemType")
              .set_display_name(u"Объект")
              .set_data_type({scada::id::BaseDataType, NamespaceIndexes::NS0}),
      .supertype_id = {scada::id::BaseVariableType, NamespaceIndexes::NS0}});
  nodes.push_back(scada::NodeState{
      .node_id = di::DataItemType_Alias,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType, NamespaceIndexes::NS0},
      .parent_id = di::DataItemType,
      .reference_type_id = {scada::id::HasProperty, NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{}
              .set_browse_name("Alias")
              .set_display_name(u"Алиас")
              .set_data_type({scada::id::String, NamespaceIndexes::NS0})});

  // Analog item type (ТИТ) and the properties tests reference.
  nodes.push_back(scada::NodeState{
      .node_id = di::AnalogItemType,
      .node_class = scada::NodeClass::VariableType,
      .parent_id = di::DataItemType,
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{}
              .set_browse_name("AnalogItemType")
              .set_display_name(u"Объект ТИТ")
              .set_data_type({scada::id::BaseDataType, NamespaceIndexes::NS0}),
      .supertype_id = di::DataItemType});
  nodes.push_back(scada::NodeState{
      .node_id = di::AnalogItemType_DisplayFormat,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType, NamespaceIndexes::NS0},
      .parent_id = di::AnalogItemType,
      .reference_type_id = {scada::id::HasProperty, NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{}
              .set_browse_name("DisplayFormat")
              .set_display_name(u"Формат")
              .set_data_type({scada::id::String, NamespaceIndexes::NS0})});

  // Discrete item type (ТС) and the properties tests reference.
  nodes.push_back(scada::NodeState{
      .node_id = di::DiscreteItemType,
      .node_class = scada::NodeClass::VariableType,
      .parent_id = di::DataItemType,
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{}
              .set_browse_name("DiscreteItemType")
              .set_display_name(u"Объект ТС")
              .set_data_type({scada::id::BaseDataType, NamespaceIndexes::NS0}),
      .supertype_id = di::DataItemType});
  nodes.push_back(scada::NodeState{
      .node_id = di::DiscreteItemType_Inversion,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType, NamespaceIndexes::NS0},
      .parent_id = di::DiscreteItemType,
      .reference_type_id = {scada::id::HasProperty, NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{}
              .set_browse_name("Inverted")
              .set_display_name(u"Инверсия")
              .set_data_type({scada::id::Boolean, NamespaceIndexes::NS0})});

  // Format type and the format-link reference type used by timed-data tests.
  nodes.push_back(scada::NodeState{
      .node_id = di::TsFormatType,
      .node_class = scada::NodeClass::ObjectType,
      .parent_id = {scada::id::BaseObjectType, NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("TsFormatType")
                        .set_display_name(u"Формат"),
      .supertype_id = {scada::id::BaseObjectType, NamespaceIndexes::NS0}});
  nodes.push_back(scada::NodeState{
      .node_id = di::TsFormatType_CloseLabel,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType, NamespaceIndexes::NS0},
      .parent_id = di::TsFormatType,
      .reference_type_id = {scada::id::HasProperty, NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("CloseLabel")
                        .set_display_name(u"Подпись 1")
                        .set_data_type({scada::id::LocalizedText,
                                        NamespaceIndexes::NS0})});

  // Data group type used by configuration/object-tree tests.
  nodes.push_back(scada::NodeState{
      .node_id = di::DataGroupType,
      .node_class = scada::NodeClass::ObjectType,
      .parent_id = {scada::id::BaseObjectType, NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("DataGroupType")
                        .set_display_name(u"Группа"),
      .supertype_id = {scada::id::BaseObjectType, NamespaceIndexes::NS0}});

  // Conversion enumeration data type referenced by AnalogItemType_Conversion.
  nodes.push_back(scada::NodeState{
      .node_id = di::AnalogConversionDataType,
      .node_class = scada::NodeClass::DataType,
      .parent_id = {scada::id::Enumeration, NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("AnalogConversionDataType")
                        .set_display_name(u"Преобразование"),
      .supertype_id = {scada::id::Enumeration, NamespaceIndexes::NS0}});
  nodes.push_back(scada::NodeState{
      .node_id = di::AnalogConversionDataType_EnumStrings,
      .node_class = scada::NodeClass::Variable,
      .type_definition_id = {scada::id::PropertyType, NamespaceIndexes::NS0},
      .parent_id = di::AnalogConversionDataType,
      .reference_type_id = {scada::id::HasProperty, NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{}
              .set_browse_name("EnumStrings")
              .set_display_name(u"EnumStrings")
              .set_data_type({scada::id::LocalizedText, NamespaceIndexes::NS0})
              .set_value(scada::Variant{
                  std::vector<scada::LocalizedText>{u"Нет", u"Линейное"}})});

  // Full DataItemType / AnalogItemType / DiscreteItemType property
  // declarations, mirroring data_items.xml (and scada_core.xml for Conversion).
  // Tests such as ExportDataReader require every property column's declaration
  // to exist.
  const scada::NodeId kString{scada::id::String, NamespaceIndexes::NS0};
  const scada::NodeId kBool{scada::id::Boolean, NamespaceIndexes::NS0};
  const scada::NodeId kInt32{scada::id::Int32, NamespaceIndexes::NS0};
  const scada::NodeId kDouble{scada::id::Double, NamespaceIndexes::NS0};
  const scada::NodeId kLocalizedText{scada::id::LocalizedText,
                                     NamespaceIndexes::NS0};

  struct PropertyDef {
    scada::NodeId id;
    scada::NodeId parent;
    const char* browse_name;
    std::u16string_view display_name;
    scada::NodeId data_type;
  };

  const PropertyDef kProperties[] = {
      // DataItemType properties.
      {di::DataItemType_Simulated, di::DataItemType, "Simulated", u"Эмуляция",
       kBool},
      {di::DataItemType_Severity, di::DataItemType, "Severity", u"Важность",
       kInt32},
      {di::DataItemType_Input1, di::DataItemType, "Input1", u"Основной ввод",
       kString},
      {di::DataItemType_Input2, di::DataItemType, "Input2", u"Резервный ввод",
       kString},
      {di::DataItemType_Output, di::DataItemType, "Output", u"Вывод", kString},
      {di::DataItemType_OutputCondition, di::DataItemType, "OutputCondition",
       u"Условие управления", kString},
      {di::DataItemType_StalePeriod, di::DataItemType, "StalePeriod",
       u"Устаревание, с", kInt32},
      {di::DataItemType_Locked, di::DataItemType, "Locked", u"Блокировка",
       kBool},
      {di::DataItemType_OutputTwoStaged, di::DataItemType, "OutputTwoStaged",
       u"Двухэтапное управление", kBool},
      // AnalogItemType properties.
      {di::AnalogItemType_Conversion, di::AnalogItemType, "Conversion",
       u"Преобразование", di::AnalogConversionDataType},
      {di::AnalogItemType_Clamping, di::AnalogItemType, "Clamping",
       u"Ограничение диапазона", kBool},
      {di::AnalogItemType_EuLo, di::AnalogItemType, "EuLo",
       u"Логический минимимум", kDouble},
      {di::AnalogItemType_EuHi, di::AnalogItemType, "EuHi",
       u"Логический максимум", kDouble},
      {di::AnalogItemType_IrLo, di::AnalogItemType, "IrLo",
       u"Физический минимимум", kDouble},
      {di::AnalogItemType_IrHi, di::AnalogItemType, "IrHi",
       u"Физический максимум", kDouble},
      {di::AnalogItemType_LimitLo, di::AnalogItemType, "LimitLo",
       u"Уставка нижняя предаварийная", kDouble},
      {di::AnalogItemType_LimitHi, di::AnalogItemType, "LimitHi",
       u"Уставка верхняя предаварийная", kDouble},
      {di::AnalogItemType_LimitLoLo, di::AnalogItemType, "LimitLoLo",
       u"Уставка нижняя аварийная", kDouble},
      {di::AnalogItemType_LimitHiHi, di::AnalogItemType, "LimitHiHi",
       u"Уставка верхняя аварийная", kDouble},
      {di::AnalogItemType_EngineeringUnits, di::AnalogItemType,
       "EngineeringUnits", u"Единицы измерения", kLocalizedText},
      {di::AnalogItemType_Aperture, di::AnalogItemType, "Aperture", u"Апертура",
       kDouble},
      {di::AnalogItemType_Deadband, di::AnalogItemType, "Deadband",
       u"Мертвая зона", kDouble},
  };

  for (const auto& prop : kProperties) {
    nodes.push_back(scada::NodeState{
        .node_id = prop.id,
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = {scada::id::PropertyType, NamespaceIndexes::NS0},
        .parent_id = prop.parent,
        .reference_type_id = {scada::id::HasProperty, NamespaceIndexes::NS0},
        .attributes =
            scada::NodeAttributes{}
                .set_browse_name(prop.browse_name)
                .set_display_name(scada::LocalizedText{prop.display_name})
                .set_data_type(prop.data_type)});
  }

  for (const auto& node : nodes) {
    [[maybe_unused]] const auto result = factory.CreateNode(node);
    base::Check(result.first);
  }
}

// Adds the `devices` test type definitions that common/client device-tree tests
// rely on (the Devices folder, the base DeviceType, and the IEC 61850 device /
// logical-node types). Display names mirror `model/nodesets/devices.xml` and
// `devices_iec61850.xml`. The standard OPC UA nodes must already be present.
inline void AddScadaDevicesTestTypes(AddressSpaceImpl& address_space) {
  GenericNodeFactory factory{address_space};

  namespace dev = devices::id;

  std::vector<scada::NodeState> nodes;

  // The "Все оборудование" folder that owns device instances.
  nodes.push_back(scada::NodeState{
      .node_id = dev::Devices,
      .node_class = scada::NodeClass::Object,
      .type_definition_id = {scada::id::FolderType, NamespaceIndexes::NS0},
      .parent_id = {scada::id::ObjectsFolder, NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::Organizes, NamespaceIndexes::NS0},
      .attributes =
          scada::NodeAttributes{}.set_browse_name("Devices").set_display_name(
              u"Все оборудование")});

  // Base device type.
  nodes.push_back(scada::NodeState{
      .node_id = dev::DeviceType,
      .node_class = scada::NodeClass::ObjectType,
      .parent_id = {scada::id::BaseObjectType, NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("DeviceType")
                        .set_display_name(u"Устройство"),
      .supertype_id = {scada::id::BaseObjectType, NamespaceIndexes::NS0}});

  // IEC 61850 device type (subtype of DeviceType) and its logical-node type.
  nodes.push_back(scada::NodeState{
      .node_id = dev::Iec61850DeviceType,
      .node_class = scada::NodeClass::ObjectType,
      .parent_id = dev::DeviceType,
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("Iec61850DeviceType")
                        .set_display_name(u"Устройство МЭК-61850"),
      .supertype_id = dev::DeviceType});
  nodes.push_back(scada::NodeState{
      .node_id = dev::Iec61850LogicalNodeType,
      .node_class = scada::NodeClass::ObjectType,
      .parent_id = {scada::id::BaseObjectType, NamespaceIndexes::NS0},
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("Iec61850LogicalNodeType")
                        .set_display_name(u"Логический узел МЭК-61850"),
      .supertype_id = {scada::id::BaseObjectType, NamespaceIndexes::NS0}});

  // IEC 60870 device type (subtype of DeviceType), used by device-metrics
  // tests.
  nodes.push_back(scada::NodeState{
      .node_id = dev::Iec60870DeviceType,
      .node_class = scada::NodeClass::ObjectType,
      .parent_id = dev::DeviceType,
      .reference_type_id = {scada::id::HasSubtype, NamespaceIndexes::NS0},
      .attributes = scada::NodeAttributes{}
                        .set_browse_name("Iec60870DeviceType")
                        .set_display_name(u"Устройство МЭК-60870"),
      .supertype_id = dev::DeviceType});

  // DeviceType metric components (HasComponent, BaseVariableType), in the order
  // device-metrics tests expect. Display names mirror devices.xml.
  struct ComponentDef {
    scada::NodeId id;
    const char* browse_name;
    std::u16string_view display_name;
  };
  const ComponentDef kDeviceMetricComponents[] = {
      {dev::DeviceType_Online, "Online", u"Связь"},
      {dev::DeviceType_Enabled, "Enabled", u"Включено"},
      {dev::DeviceType_MessagesIn, "MessagesIn", u"Принято сообщений"},
      {dev::DeviceType_MessagesOut, "MessagesOut", u"Отправлено сообщений"},
      {dev::DeviceType_BytesIn, "BytesIn", u"Принято байт"},
      {dev::DeviceType_BytesOut, "BytesOut", u"Отправлено байт"},
      {dev::DeviceType_SyncClockCount, "SyncClockCount",
       u"Число синхронизаций времени"},
      {dev::DeviceType_InterrogateCount, "InterrogateCount",
       u"Число полных опросов"},
  };
  for (const auto& component : kDeviceMetricComponents) {
    nodes.push_back(scada::NodeState{
        .node_id = component.id,
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = {scada::id::BaseVariableType,
                               NamespaceIndexes::NS0},
        .parent_id = dev::DeviceType,
        .reference_type_id = {scada::id::HasComponent, NamespaceIndexes::NS0},
        .attributes = scada::NodeAttributes{}
                          .set_browse_name(component.browse_name)
                          .set_display_name(
                              scada::LocalizedText{component.display_name})});
  }

  for (const auto& node : nodes) {
    [[maybe_unused]] const auto result = factory.CreateNode(node);
    base::Check(result.first);
  }
}

// A ready-to-use code-defined SCADA address space: the standard OPC UA tree
// plus the data_items and devices test type systems. Replaces
// `AddressSpaceImpl3` for tests without loading any nodeset XML.
//
// `StandardAddressSpace` owns its standard nodes as members, so the address
// space must drop its node references (Clear()) before those members are
// destroyed — the destructor does this, mirroring `TestAddressSpace`.
class ScadaTestAddressSpace : public AddressSpaceImpl {
 public:
  ScadaTestAddressSpace() {
    AddScadaDataItemsTestTypes(*this);
    AddScadaDevicesTestTypes(*this);

    // The "Creates" reference type declares which instance types a container
    // type may hold (used by child-property/type resolution). ReferenceType
    // nodes cannot be built by GenericNodeFactory, so add it directly: a data
    // group may contain data items.
    AddStaticNode<scada::ReferenceType>(scada::id::Creates, "Creates");
    scada::AddReference(*this, scada::id::Creates,
                        data_items::id::DataGroupType,
                        data_items::id::DataItemType);

    // Non-hierarchical reference types used as data-item property columns (e.g.
    // by ExportDataReader). Also ReferenceType, so added directly.
    AddStaticNode<scada::ReferenceType>(data_items::id::HasSimulationSignal,
                                        "HasSimulationSignal");
    AddStaticNode<scada::ReferenceType>(history::id::HasHistoricalDatabase,
                                        "HasHistoricalDatabase");
    AddStaticNode<scada::ReferenceType>(data_items::id::HasTsFormat,
                                        "HasTsFormat");
  }

  ~ScadaTestAddressSpace() { Clear(); }

 private:
  StandardAddressSpace standard_address_space_{*this};
};

}  // namespace scada_test
