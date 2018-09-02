#include "address_space/scada_address_space.h"

#include "address_space/address_space_impl.h"
#include "address_space/node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/strings/utf_string_conversions.h"
#include "common/node_state.h"
#include "common/scada_node_ids.h"
#include "core/privileges.h"

namespace {

scada::ObjectType* CreateObjectType(AddressSpaceImpl& address_space,
                                    const scada::NodeId& id,
                                    scada::QualifiedName qualified_name,
                                    scada::LocalizedText display_name,
                                    const scada::NodeId& supertype_id) {
  assert(!address_space.GetNode(id));

  auto type = std::make_unique<scada::ObjectType>(id, std::move(qualified_name),
                                                  std::move(display_name));
  auto* result = type.get();
  address_space.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = address_space.GetNode(supertype_id);
    assert(supertype &&
           supertype->GetNodeClass() == scada::NodeClass::ObjectType);
    AddReference(address_space, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::VariableType* CreateVariableType(AddressSpaceImpl& address_space,
                                        const scada::NodeId& id,
                                        scada::QualifiedName browse_name,
                                        scada::LocalizedText display_name,
                                        const scada::NodeId& data_type_id,
                                        const scada::NodeId& supertype_id) {
  assert(!address_space.GetNode(id));

  auto* data_type = address_space.GetNode(data_type_id);
  assert(data_type && data_type->GetNodeClass() == scada::NodeClass::DataType);

  auto type = std::make_unique<scada::VariableType>(
      id, std::move(browse_name), std::move(display_name),
      *static_cast<const scada::DataType*>(data_type));
  auto* result = type.get();
  address_space.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = address_space.GetNode(supertype_id);
    assert(supertype &&
           supertype->GetNodeClass() == scada::NodeClass::VariableType);
    AddReference(address_space, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::DataType* CreateDataType(AddressSpaceImpl& address_space,
                                const scada::NodeId& id,
                                scada::QualifiedName browse_name,
                                scada::LocalizedText display_name,
                                const scada::NodeId& supertype_id) {
  assert(!address_space.GetNode(id));

  std::unique_ptr<scada::DataType> type(
      new scada::DataType(id, std::move(browse_name), std::move(display_name)));
  auto* result = type.get();
  address_space.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = address_space.GetNode(supertype_id);
    assert(supertype &&
           supertype->GetNodeClass() == scada::NodeClass::DataType);
    AddReference(address_space, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::Variable* AddProperty(AddressSpaceImpl& address_space,
                             const scada::NodeId& type_id,
                             const scada::NodeId& prop_type_id,
                             scada::QualifiedName browse_name,
                             scada::LocalizedText display_name,
                             const scada::NodeId& data_type_id,
                             scada::Variant default_value = scada::Variant()) {
  auto* type = address_space.GetNode(type_id);
  assert(type && (type->GetNodeClass() == scada::NodeClass::ObjectType ||
                  type->GetNodeClass() == scada::NodeClass::VariableType));

  auto* existing_prop = address_space.GetNode(prop_type_id);
  assert(!existing_prop ||
         existing_prop->GetNodeClass() == scada::NodeClass::Variable);

  auto* result = static_cast<scada::Variable*>(existing_prop);
  if (!result) {
    auto* data_type = AsDataType(address_space.GetNode(data_type_id));
    assert(data_type);

    auto variable = std::make_unique<scada::GenericVariable>(
        prop_type_id, std::move(browse_name), std::move(display_name),
        *data_type);
    variable->SetValue({default_value, {}, {}, {}});
    result = variable.get();
    address_space.AddStaticNode(std::move(variable));

    AddReference(address_space, scada::id::HasTypeDefinition, prop_type_id,
                 scada::id::PropertyType);
    AddReference(address_space, scada::id::HasModellingRule, prop_type_id,
                 scada::id::ModellingRule_Mandatory);
  }

  AddReference(address_space, scada::id::HasProperty, *type, *result);
  return result;
}

scada::ReferenceType* CreateReferenceType(AddressSpaceImpl& address_space,
                                          const scada::NodeId& source_type_id,
                                          const scada::NodeId& prop_type_id,
                                          scada::QualifiedName browse_name,
                                          scada::LocalizedText display_name,
                                          const scada::NodeId& target_type_id) {
  auto* source_type = address_space.GetNode(source_type_id);
  assert(source_type &&
         (source_type->GetNodeClass() == scada::NodeClass::ObjectType ||
          source_type->GetNodeClass() == scada::NodeClass::VariableType));

  auto* target_type = address_space.GetNode(target_type_id);
  assert(target_type);

  auto* existing_prop = address_space.GetNode(prop_type_id);
  assert(!existing_prop ||
         existing_prop->GetNodeClass() == scada::NodeClass::ReferenceType);

  auto* result = static_cast<scada::ReferenceType*>(existing_prop);
  if (!result) {
    auto ref_type = std::make_unique<scada::ReferenceType>(
        prop_type_id, std::move(browse_name), std::move(display_name));
    result = ref_type.get();
    address_space.AddStaticNode(std::move(ref_type));
    AddReference(address_space, scada::id::HasSubtype,
                 scada::id::NonHierarchicalReferences, prop_type_id);
  }

  AddReference(*result, *source_type, *target_type);
  return result;
}

void AddDataVariable(AddressSpaceImpl& address_space,
                     const scada::NodeId& type_id,
                     const scada::NodeId& variable_decl_id,
                     scada::QualifiedName browse_name,
                     scada::LocalizedText display_name,
                     const scada::NodeId& data_type_id,
                     scada::Variant default_value = scada::Variant()) {
  auto* type = address_space.GetNode(type_id);
  assert(type);
  assert(type->GetNodeClass() == scada::NodeClass::ObjectType ||
         type->GetNodeClass() == scada::NodeClass::VariableType);

  auto* existing_prop = address_space.GetNode(variable_decl_id);
  assert(!existing_prop ||
         existing_prop->GetNodeClass() == scada::NodeClass::Variable);

  auto* result = static_cast<scada::Variable*>(existing_prop);
  if (!result) {
    auto* variable_type = scada::AsVariableType(
        address_space.GetNode(scada::id::BaseVariableType));
    assert(variable_type);
    auto* data_type = scada::AsDataType(address_space.GetNode(data_type_id));
    assert(data_type);
    auto variable = std::make_unique<scada::GenericVariable>(
        variable_decl_id, std::move(browse_name), std::move(display_name),
        *data_type);
    auto now = base::Time::Now();
    variable->SetValue({std::move(default_value), {}, now, now});
    result = variable.get();
    address_space.AddStaticNode(std::move(variable));
    AddReference(address_space, scada::id::HasTypeDefinition, *result,
                 *variable_type);
    AddReference(address_space, scada::id::HasModellingRule, variable_decl_id,
                 scada::id::ModellingRule_Mandatory);
  }

  AddReference(address_space, scada::id::HasComponent, *type, *result);
}

}  // namespace

StaticAddressSpace::DeviceType::DeviceType(StandardAddressSpace& std,
                                           scada::NodeId id,
                                           scada::QualifiedName browse_name,
                                           scada::LocalizedText display_name)
    : scada::ObjectType(id, std::move(browse_name), std::move(display_name)),
      Disabled{std,
               id::DeviceType_Disabled,
               "Disabled",
               base::WideToUTF16(L"Отключено"),
               std.BoolDataType,
               true},
      Online{std,
             id::DeviceType_Online,
             "Online",
             base::WideToUTF16(L"Связь"),
             std.BaseVariableType,
             std.BoolDataType,
             false},
      Enabled{std,
              id::DeviceType_Enabled,
              "Enabled",
              base::WideToUTF16(L"Включено"),
              std.BaseVariableType,
              std.BoolDataType,
              false} {
  SetDisplayName(std::move(display_name));
  scada::AddReference(std.HasSubtype, std.BaseObjectType, *this);
  scada::AddReference(std.HasProperty, *this, Disabled);
  scada::AddReference(std.HasComponent, *this, Online);
  scada::AddReference(std.HasComponent, *this, Enabled);
}

StaticAddressSpace::StaticAddressSpace(StandardAddressSpace& std)
    : DeviceType{std, id::DeviceType, "DeviceType",
                 base::WideToUTF16(L"Устройство")} {
  scada::AddReference(std.HasSubtype, std.NonHierarchicalReference, Creates);
}

void CreateScadaAddressSpace(AddressSpaceImpl& address_space,
                             NodeFactory& node_factory) {
  node_factory.CreateNode(
      {id::DataItems, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}
           .set_browse_name("DataItems")
           .set_display_name(base::WideToUTF16(L"Все объекты"))});

  node_factory.CreateNode(
      {id::Devices, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}.set_browse_name("Devices").set_display_name(
           base::WideToUTF16(L"Все оборудование"))});

  node_factory.CreateNode(
      {id::Users, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}.set_browse_name("Users").set_display_name(
           base::WideToUTF16(L"Пользователи"))});

  node_factory.CreateNode(
      {id::TsFormats, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}
           .set_browse_name("TsFormats")
           .set_display_name(base::WideToUTF16(L"Форматы"))});

  node_factory.CreateNode(
      {id::SimulationSignals, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}
           .set_browse_name("SimulationSignals")
           .set_display_name(base::WideToUTF16(L"Эмулируемые сигналы"))});

  node_factory.CreateNode(
      {id::HistoricalDatabases, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}
           .set_browse_name("HistoricalDatabases")
           .set_display_name(base::WideToUTF16(L"Базы данных"))});

  node_factory.CreateNode(
      {id::TransmissionItems, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}
           .set_browse_name("TransmissionItems")
           .set_display_name(base::WideToUTF16(L"Ретрансляция"))});

  // Link
  {
    CreateObjectType(address_space, id::LinkType, "LinkType",
                     base::WideToUTF16(L"Направление"), id::DeviceType);
    AddReference(address_space, id::Creates, id::Devices, id::LinkType);
    AddProperty(address_space, id::LinkType, id::LinkType_Transport,
                "TransportString", base::WideToUTF16(L"Транспорт"),
                scada::id::String);
    AddDataVariable(address_space, id::LinkType, id::LinkType_ConnectCount,
                    "ConnectCount", base::WideToUTF16(L"ConnectCount"),
                    scada::id::Int32, 0);
    AddDataVariable(address_space, id::LinkType, id::LinkType_ActiveConnections,
                    "ActiveConnections",
                    base::WideToUTF16(L"ActiveConnections"), scada::id::Int32,
                    0);
    AddDataVariable(address_space, id::LinkType, id::LinkType_MessagesOut,
                    "MessagesOut", base::WideToUTF16(L"MessagesOut"),
                    scada::id::Int32, 0);
    AddDataVariable(address_space, id::LinkType, id::LinkType_MessagesIn,
                    "MessagesIn", base::WideToUTF16(L"MessagesIn"),
                    scada::id::Int32, 0);
    AddDataVariable(address_space, id::LinkType, id::LinkType_BytesOut,
                    "BytesOut", base::WideToUTF16(L"BytesOut"),
                    scada::id::Int32, 0);
    AddDataVariable(address_space, id::LinkType, id::LinkType_BytesIn,
                    "BytesIn", base::WideToUTF16(L"BytesIn"), scada::id::Int32,
                    0);
  }

  // Simulation Item
  {
    CreateVariableType(address_space, id::SimulationSignalType,
                       "SimulationSignalType",
                       base::WideToUTF16(L"Эмулируемый сигнал"),
                       scada::id::BaseDataType, scada::id::BaseVariableType);
    AddReference(address_space, id::Creates, id::SimulationSignals,
                 id::SimulationSignalType);
    AddProperty(address_space, id::SimulationSignalType,
                id::SimulationSignalType_Type, "Type",
                base::WideToUTF16(L"Тип"), scada::id::Int32, 0);
    AddProperty(address_space, id::SimulationSignalType,
                id::SimulationSignalType_Period, "Period",
                base::WideToUTF16(L"Период (мс)"), scada::id::Int32, 60000);
    AddProperty(address_space, id::SimulationSignalType,
                id::SimulationSignalType_Phase, "Phase",
                base::WideToUTF16(L"Фаза (мс)"), scada::id::Int32, 0);
    AddProperty(address_space, id::SimulationSignalType,
                id::SimulationSignalType_UpdateInterval, "UpdateInterval",
                base::WideToUTF16(L"Обновление (мс)"), scada::id::Int32, 1000);
  }

  // Historical DB
  {
    CreateObjectType(
        address_space, id::HistoricalDatabaseType, "HistoricalDatabaseType",
        base::WideToUTF16(L"База данных"), scada::id::BaseObjectType);
    AddReference(address_space, id::Creates, id::HistoricalDatabases,
                 id::HistoricalDatabaseType);
    AddProperty(address_space, id::HistoricalDatabaseType,
                id::HistoricalDatabaseType_Depth, "Depth",
                base::WideToUTF16(L"Глубина (дн)"), scada::id::Int32, 1);
    AddDataVariable(address_space, id::HistoricalDatabaseType,
                    id::HistoricalDatabaseType_WriteValueDuration,
                    "WriteValueDuration",
                    base::WideToUTF16(L"WriteValueDuration"), scada::id::Int32);
    AddDataVariable(address_space, id::HistoricalDatabaseType,
                    id::HistoricalDatabaseType_PendingTaskCount,
                    "PendingTaskCount", base::WideToUTF16(L"PendingTaskCount"),
                    scada::id::Int32);
    AddDataVariable(
        address_space, id::HistoricalDatabaseType,
        id::HistoricalDatabaseType_EventCleanupDuration, "EventCleanupDuration",
        base::WideToUTF16(L"EventCleanupDuration"), scada::id::Int32);
    AddDataVariable(
        address_space, id::HistoricalDatabaseType,
        id::HistoricalDatabaseType_ValueCleanupDuration, "ValueCleanupDuration",
        base::WideToUTF16(L"ValueCleanupDuration"), scada::id::Int32);
  }

  // System historical database.
  node_factory.CreateNode(
      {id::SystemDatabase,
       scada::NodeClass::Object,
       id::HistoricalDatabaseType,
       id::HistoricalDatabases,
       scada::id::Organizes,
       scada::NodeAttributes().set_browse_name("System").set_display_name(
           base::WideToUTF16(L"Системная база данных")),
       {{id::HistoricalDatabaseType_Depth, 30}}});

  // TsFormat
  {
    CreateObjectType(address_space, id::TsFormatType, "TsFormatType",
                     base::WideToUTF16(L"Формат"), scada::id::BaseObjectType);
    AddReference(address_space, id::Creates, id::TsFormats, id::TsFormatType);
    AddProperty(address_space, id::TsFormatType, id::TsFormatType_OpenLabel,
                "OpenLabel", base::WideToUTF16(L"Подпись 0"),
                scada::id::LocalizedText);
    AddProperty(address_space, id::TsFormatType, id::TsFormatType_CloseLabel,
                "CloseLabel", base::WideToUTF16(L"Подпись 1"),
                scada::id::LocalizedText);
    AddProperty(address_space, id::TsFormatType, id::TsFormatType_OpenColor,
                "OpenColor", base::WideToUTF16(L"Цвет 0"), scada::id::Int32);
    AddProperty(address_space, id::TsFormatType, id::TsFormatType_CloseColor,
                "CloseColor", base::WideToUTF16(L"Цвет 1"), scada::id::Int32);
  }

  // DataGroup
  {
    CreateObjectType(address_space, id::DataGroupType, "DataGroupType",
                     base::WideToUTF16(L"Группа"), scada::id::BaseObjectType);
    AddReference(address_space, id::Creates, id::DataItems, id::DataGroupType);
    AddReference(address_space, id::Creates, id::DataGroupType,
                 id::DataGroupType);  // recursive
    AddProperty(address_space, id::DataGroupType, id::DataGroupType_Simulated,
                "Simulated", base::WideToUTF16(L"Эмуляция"), scada::id::Boolean,
                false);
  }

  // DataItem
  {
    CreateVariableType(address_space, id::DataItemType, "DataItemType",
                       base::WideToUTF16(L"Объект"), scada::id::BaseDataType,
                       scada::id::BaseVariableType);
    AddReference(address_space, id::Creates, id::DataGroupType,
                 id::DataItemType);
    AddProperty(address_space, id::DataItemType, id::DataItemType_Alias,
                "Alias", base::WideToUTF16(L"Алиас"), scada::id::String);
    AddProperty(address_space, id::DataItemType, id::DataItemType_Severity,
                "Severity", base::WideToUTF16(L"Важность"), scada::id::Int32,
                1);
    AddProperty(address_space, id::DataItemType, id::DataItemType_Input1,
                "Input1", base::WideToUTF16(L"Ввод1"), scada::id::String);
    AddProperty(address_space, id::DataItemType, id::DataItemType_Input2,
                "Input2", base::WideToUTF16(L"Ввод2"), scada::id::String);
    AddProperty(address_space, id::DataItemType, id::DataItemType_Output,
                "Output", base::WideToUTF16(L"Вывод"), scada::id::String);
    AddProperty(address_space, id::DataItemType,
                id::DataItemType_OutputCondition, "OutputCondition",
                base::WideToUTF16(L"Условие (Управление)"), scada::id::String);
    AddProperty(address_space, id::DataItemType, id::DataItemType_StalePeriod,
                "StalePeriod", base::WideToUTF16(L"Устаревание (с)"),
                scada::id::Int32, 0);
    AddProperty(address_space, id::DataItemType, id::DataItemType_Simulated,
                "Simulated", base::WideToUTF16(L"Эмуляция"), scada::id::Boolean,
                false);
    CreateReferenceType(address_space, id::DataItemType,
                        id::HasSimulationSignal, "HasSimulationSignal",
                        base::WideToUTF16(L"Эмулируемый сигнал"),
                        id::SimulationSignalType);
    AddProperty(address_space, id::DataItemType, id::DataItemType_Locked,
                "Locked", base::WideToUTF16(L"Блокирован"), scada::id::Boolean,
                false);
    CreateReferenceType(address_space, id::DataItemType,
                        id::HasHistoricalDatabase, "ObjectHistoricalDb",
                        base::WideToUTF16(L"База данных"),
                        id::HistoricalDatabaseType);
  }

  // DiscreteItem
  {
    CreateVariableType(address_space, id::DiscreteItemType, "DiscreteItemType",
                       base::WideToUTF16(L"Объект ТС"), scada::id::BaseDataType,
                       id::DataItemType);
    AddProperty(address_space, id::DiscreteItemType,
                id::DiscreteItemType_Inversion, "Inverted",
                base::WideToUTF16(L"Инверсия"), scada::id::Boolean, false);
    CreateReferenceType(address_space, id::DiscreteItemType, id::HasTsFormat,
                        "HasTsFormat", base::WideToUTF16(L"Параметры"),
                        id::TsFormatType);
  }

  // AnalogItem
  {
    CreateVariableType(address_space, id::AnalogItemType, "AnalogItemType",
                       base::WideToUTF16(L"Объект ТИТ"),
                       scada::id::BaseDataType, id::DataItemType);
    AddProperty(address_space, id::AnalogItemType,
                id::AnalogItemType_DisplayFormat, "DisplayFormat",
                base::WideToUTF16(L"Формат"), scada::id::String, "0.####");
    AddProperty(address_space, id::AnalogItemType,
                id::AnalogItemType_Conversion, "ConversionType",
                base::WideToUTF16(L"Преобразование"), scada::id::Int32, 0);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_Clamping,
                "ClampingType", base::WideToUTF16(L"Ограничение диапазона"),
                scada::id::Boolean, false);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_EuLo,
                "EuLo", base::WideToUTF16(L"Лог мин"), scada::id::Double, -100);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_EuHi,
                "EuHi", base::WideToUTF16(L"Лог макс"), scada::id::Double, 100);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_IrLo,
                "IrLo", base::WideToUTF16(L"Физ мин"), scada::id::Double, 0);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_IrHi,
                "IrHi", base::WideToUTF16(L"Физ макс"), scada::id::Double,
                65535);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_LimitLo,
                "LimitLo", base::WideToUTF16(L"Уставка нижняя предаварийная"),
                scada::id::Double);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_LimitHi,
                "LimitHi", base::WideToUTF16(L"Уставка верхняя предаварийная"),
                scada::id::Double);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_LimitLoLo,
                "LimitLoLo", base::WideToUTF16(L"Уставка нижняя аварийная"),
                scada::id::Double);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_LimitHiHi,
                "LimitHiHi", base::WideToUTF16(L"Уставка верхняя аварийная"),
                scada::id::Double);
    AddProperty(address_space, id::AnalogItemType,
                id::AnalogItemType_EngineeringUnits, "EngineeringUnits",
                base::WideToUTF16(L"Единицы измерения"),
                scada::id::LocalizedText);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_Aperture,
                "Aperture", base::WideToUTF16(L"Апертура"), scada::id::Double,
                0.0);
    AddProperty(address_space, id::AnalogItemType, id::AnalogItemType_Deadband,
                "Deadband", base::WideToUTF16(L"Мертвая зона"),
                scada::id::Double, 0.0);
  }

  // Modbus Port Device
  {
    CreateObjectType(address_space, id::ModbusLinkType, "ModbusLinkType",
                     base::WideToUTF16(L"Направление MODBUS"), id::LinkType);
    auto* mode_type = CreateDataType(
        address_space, id::ModbusLinkType_Mode, "ModbusPortModeType",
        base::WideToUTF16(L"Тип направления MODBUS"), scada::id::Int32);
    mode_type->enum_strings = {base::WideToUTF16(L"Опрос"),
                               base::WideToUTF16(L"Ретрансляция"),
                               base::WideToUTF16(L"Прослушка")};
    AddProperty(address_space, id::ModbusLinkType, id::ModbusLinkType_Protocol,
                "Mode", base::WideToUTF16(L"Режим"), id::ModbusLinkType_Mode,
                0);
  }

  // Modbus Device
  {
    CreateObjectType(address_space, id::ModbusDeviceType, "ModbusDeviceType",
                     base::WideToUTF16(L"Устройство MODBUS"), id::DeviceType);
    AddReference(address_space, id::Creates, id::ModbusLinkType,
                 id::ModbusDeviceType);
    AddProperty(address_space, id::ModbusDeviceType,
                id::ModbusDeviceType_Address, "Address",
                base::WideToUTF16(L"Адрес"), scada::id::Int32, 1);
    AddProperty(address_space, id::ModbusDeviceType,
                id::ModbusDeviceType_SendRetryCount, "RepeatCount",
                base::WideToUTF16(L"Попыток повторов передачи"),
                scada::id::Int32, 3);
  }

  // User
  {
    CreateObjectType(address_space, id::UserType, "UserType",
                     base::WideToUTF16(L"Пользователь"),
                     scada::id::BaseObjectType);
    AddReference(address_space, id::Creates, id::Users, id::UserType);
    AddProperty(address_space, id::UserType, id::UserType_AccessRights,
                "AccessRights", base::WideToUTF16(L"Права"), scada::id::Int32,
                0);
  }

  // Root user.
  int root_privileges = (1 << static_cast<int>(scada::Privilege::Configure)) |
                        (1 << static_cast<int>(scada::Privilege::Control));
  node_factory.CreateNode(
      {id::RootUser,
       scada::NodeClass::Object,
       id::UserType,
       id::Users,
       scada::id::Organizes,
       scada::NodeAttributes().set_display_name(base::WideToUTF16(L"root")),
       {{id::UserType_AccessRights, root_privileges}}});

  // IEC-60870 Link Device
  {
    auto* type = CreateObjectType(
        address_space, id::Iec60870LinkType, "Iec60870LinkType",
        base::WideToUTF16(L"Направление МЭК-60870"), id::LinkType);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_Protocol, "Protocol",
                base::WideToUTF16(L"Протокол"), scada::id::Int32, 0);
    AddProperty(address_space, id::Iec60870LinkType, id::Iec60870LinkType_Mode,
                "Mode", base::WideToUTF16(L"Режим"), scada::id::Int32, 0);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_SendQueueSize, "SendQueueSize",
                base::WideToUTF16(L"Очередь передачи (K)"), scada::id::Int32,
                12);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_ReceiveQueueSize, "ReceiveQueueSize",
                base::WideToUTF16(L"Очередь приема (W)"), scada::id::Int32, 8);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_ConnectTimeout, "ConnectTimeout",
                base::WideToUTF16(L"Таймаут связи (T0, с)"), scada::id::Int32,
                30);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_ConfirmationTimeout, "ConfirmationTimeout",
                base::WideToUTF16(L"Таймаут подтверждения (с)"),
                scada::id::Int32, 5);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_TerminationTimeout, "TerminationTimeout",
                base::WideToUTF16(L"Таймаут операции (с)"), scada::id::Int32,
                20);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_DeviceAddressSize, "DeviceAddressSize",
                base::WideToUTF16(L"Размер адреса устройства"),
                scada::id::Int32, 2);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_COTSize, "CotSize",
                base::WideToUTF16(L"Размер причины передачи"), scada::id::Int32,
                2);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_InfoAddressSize, "InfoAddressSize",
                base::WideToUTF16(L"Размер адреса объекта"), scada::id::Int32,
                3);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_DataCollection, "CollectData",
                base::WideToUTF16(L"Сбор данных"), scada::id::Boolean, true);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_SendRetryCount, "SendRetries",
                base::WideToUTF16(L"Попыток повторов передачи"),
                scada::id::Int32, 0);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_CRCProtection, "CrcProtection",
                base::WideToUTF16(L"Защита CRC"), scada::id::Boolean, false);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_SendTimeout, "SendTimeout",
                base::WideToUTF16(L"Таймаут передачи (T1, с)"),
                scada::id::Int32, 5);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_ReceiveTimeout, "ReceiveTimeout",
                base::WideToUTF16(L"Таймаут приема (T2, с)"), scada::id::Int32,
                10);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_IdleTimeout, "IdleTimeout",
                base::WideToUTF16(L"Таймаут простоя (T3, с)"), scada::id::Int32,
                30);
    AddProperty(address_space, id::Iec60870LinkType,
                id::Iec60870LinkType_AnonymousMode, "AnonymousMode",
                base::WideToUTF16(L"Анонимный режим"), scada::id::Boolean);
  }

  // IEC-60870 Device
  {
    CreateObjectType(
        address_space, id::Iec60870DeviceType, "Iec60870DeviceType",
        base::WideToUTF16(L"Устройство МЭК-60870"), id::DeviceType);
    AddReference(address_space, id::Creates, id::Iec60870LinkType,
                 id::Iec60870DeviceType);
    AddProperty(address_space, id::Iec60870DeviceType,
                id::Iec60870DeviceType_Address, "Address",
                base::WideToUTF16(L"Адрес"), scada::id::Int32, 1);
    AddProperty(address_space, id::Iec60870DeviceType,
                id::Iec60870DeviceType_LinkAddress, "LinkAddress",
                base::WideToUTF16(L"Адрес коммутатора"), scada::id::Int32, 1);
    AddProperty(address_space, id::Iec60870DeviceType,
                id::Iec60870DeviceType_StartupInterrogation,
                "InterrogateOnStart",
                base::WideToUTF16(L"Полный опрос при запуске"),
                scada::id::Boolean, true);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriod, "InterrogationPeriod",
        base::WideToUTF16(L"Период полного опроса (с)"), scada::id::Int32, 0);
    AddProperty(address_space, id::Iec60870DeviceType,
                id::Iec60870DeviceType_StartupClockSync,
                "SynchronizeClockOnStart",
                base::WideToUTF16(L"Синхронизация часов при запуске"),
                scada::id::Boolean, false);
    AddProperty(address_space, id::Iec60870DeviceType,
                id::Iec60870DeviceType_ClockSyncPeriod,
                "ClockSynchronizationPeriod",
                base::WideToUTF16(L"Период синхронизации часов (с)"),
                scada::id::Int32, 0);
    AddProperty(address_space, id::Iec60870DeviceType,
                id::Iec60870DeviceType_UtcTime, "UtcTime",
                base::WideToUTF16(L"Время UTC"), scada::id::Boolean, false);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup1, "Group1PollPeriod",
        base::WideToUTF16(L"Период опроса группы 1 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup2, "Group2PollPeriod",
        base::WideToUTF16(L"Период опроса группы 2 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup3, "Group3PollPeriod",
        base::WideToUTF16(L"Период опроса группы 3 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup4, "Group4PollPeriod",
        base::WideToUTF16(L"Период опроса группы 4 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup5, "Group5PollPeriod",
        base::WideToUTF16(L"Период опроса группы 5 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup6, "Group6PollPeriod",
        base::WideToUTF16(L"Период опроса группы 6 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup7, "Group7PollPeriod",
        base::WideToUTF16(L"Период опроса группы 7 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup8, "Group8PollPeriod",
        base::WideToUTF16(L"Период опроса группы 8 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup9, "Group9PollPeriod",
        base::WideToUTF16(L"Период опроса группы 9 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup10, "Group10PollPeriod",
        base::WideToUTF16(L"Период опроса группы 10 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup11, "Group11PollPeriod",
        base::WideToUTF16(L"Период опроса группы 11 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup12, "Group12PollPeriod",
        base::WideToUTF16(L"Период опроса группы 12 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup13, "Group13PollPeriod",
        base::WideToUTF16(L"Период опроса группы 13 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup14, "Group14PollPeriod",
        base::WideToUTF16(L"Период опроса группы 14 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup15, "Group15PollPeriod",
        base::WideToUTF16(L"Период опроса группы 15 (с)"), scada::id::Int32, 0);
    AddProperty(
        address_space, id::Iec60870DeviceType,
        id::Iec60870DeviceType_InterrogationPeriodGroup16, "Group16PollPeriod",
        base::WideToUTF16(L"Период опроса группы 16 (с)"), scada::id::Int32, 0);
  }

  // IEC-61850 Logical Node
  {
    CreateObjectType(address_space, id::Iec61850LogicalNodeType,
                     "Iec61850LogicalNodeType",
                     base::WideToUTF16(L"Логический узел МЭК-61850"),
                     scada::id::BaseObjectType);
  }

  // IEC-61850 Data Variable
  {
    CreateVariableType(address_space, id::Iec61850DataVariableType,
                       "Iec61850DataVariableType",
                       base::WideToUTF16(L"Объект данных МЭК-61850"),
                       scada::id::BaseDataType, scada::id::BaseVariableType);
  }

  // IEC-61850 Control Object
  {
    CreateObjectType(address_space, id::Iec61850ControlObjectType,
                     "Iec61850ControlObjectType",
                     base::WideToUTF16(L"Объект управления МЭК-61850"),
                     scada::id::BaseObjectType);
  }

  // IEC-61850 Device
  {
    CreateObjectType(
        address_space, id::Iec61850DeviceType, "Iec61850DeviceType",
        base::WideToUTF16(L"Устройство МЭК-61850"), id::DeviceType);
    AddReference(address_space, id::Creates, id::Devices,
                 id::Iec61850DeviceType);
    {
      auto model = std::make_unique<scada::GenericObject>();
      model->set_id(id::Iec61850DeviceType_Model);
      model->SetBrowseName("Model");
      model->SetDisplayName(base::WideToUTF16(L"Модель"));
      address_space.AddStaticNode(std::move(model));
      AddReference(address_space, scada::id::HasTypeDefinition,
                   id::Iec61850DeviceType_Model, id::Iec61850LogicalNodeType);
      AddReference(address_space, scada::id::HasModellingRule,
                   id::Iec61850DeviceType_Model,
                   scada::id::ModellingRule_Mandatory);
      // Only HasComponent can be nested now.
      AddReference(address_space, scada::id::HasComponent,
                   id::Iec61850DeviceType, id::Iec61850DeviceType_Model);
    }
    AddProperty(address_space, id::Iec61850DeviceType,
                id::Iec61850DeviceType_Host, "Host", base::WideToUTF16(L"Хост"),
                scada::id::String);
    AddProperty(address_space, id::Iec61850DeviceType,
                id::Iec61850DeviceType_Port, "Port", base::WideToUTF16(L"Порт"),
                scada::id::Int32, 102);
  }

  // IEC-61850 ConfigurableObject
  {
    CreateObjectType(
        address_space, id::Iec61850ConfigurableObjectType, "ConfigurableObject",
        base::WideToUTF16(L"Параметр МЭК-61850"), scada::id::BaseObjectType);
    AddReference(address_space, id::Creates, id::Iec61850ConfigurableObjectType,
                 id::Iec61850ConfigurableObjectType);
    AddProperty(address_space, id::Iec61850ConfigurableObjectType,
                id::Iec61850ConfigurableObjectType_Reference, "Address",
                base::WideToUTF16(L"Адрес"), scada::id::String);
  }

  // IEC-61850 RCB
  {
    CreateObjectType(address_space, id::Iec61850RcbType, "RCB",
                     base::WideToUTF16(L"RCB МЭК-61850"),
                     id::Iec61850ConfigurableObjectType);
    AddReference(address_space, id::Creates, id::Iec61850DeviceType,
                 id::Iec61850RcbType);
  }

  // IEC Retransmission Item
  {
    CreateObjectType(address_space, id::TransmissionItemType,
                     "TransmissionItemType", base::WideToUTF16(L"Ретрансляция"),
                     scada::id::BaseObjectType);
    AddReference(address_space, id::Creates, id::TransmissionItems,
                 id::TransmissionItemType);
    CreateReferenceType(address_space, id::TransmissionItemType,
                        id::HasTransmissionSource, "IecTransmitSource",
                        base::WideToUTF16(L"Объект источник"),
                        scada::id::BaseVariableType);
    CreateReferenceType(address_space, id::TransmissionItemType,
                        id::HasTransmissionTarget, "IecTransmitTargetDevice",
                        base::WideToUTF16(L"Устройство приемник"),
                        id::DeviceType);
    AddProperty(address_space, id::TransmissionItemType,
                id::TransmissionItemType_SourceAddress, "InfoAddress",
                base::WideToUTF16(L"Адрес объекта приемника"), scada::id::Int32,
                1);
  }
}
