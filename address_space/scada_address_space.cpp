#include "address_space/scada_address_space.h"

#include "common/node_state.h"
#include "common/scada_node_ids.h"
#include "address_space/address_space_impl.h"
#include "address_space/node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "core/privileges.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"

namespace {

scada::ObjectType* CreateObjectType(AddressSpaceImpl& configuration,
                                    const scada::NodeId& id,
                                    scada::QualifiedName qualified_name,
                                    scada::LocalizedText display_name,
                                    const scada::NodeId& supertype_id) {
  assert(!configuration.GetNode(id));

  auto type = std::make_unique<scada::ObjectType>(id, std::move(qualified_name),
                                                  std::move(display_name));
  auto* result = type.get();
  configuration.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = configuration.GetNode(supertype_id);
    assert(supertype &&
           supertype->GetNodeClass() == scada::NodeClass::ObjectType);
    AddReference(configuration, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::VariableType* CreateVariableType(AddressSpaceImpl& configuration,
                                        const scada::NodeId& id,
                                        scada::QualifiedName browse_name,
                                        scada::LocalizedText display_name,
                                        const scada::NodeId& data_type_id,
                                        const scada::NodeId& supertype_id) {
  assert(!configuration.GetNode(id));

  auto* data_type = configuration.GetNode(data_type_id);
  assert(data_type && data_type->GetNodeClass() == scada::NodeClass::DataType);

  auto type = std::make_unique<scada::VariableType>(
      id, std::move(browse_name), std::move(display_name),
      *static_cast<const scada::DataType*>(data_type));
  auto* result = type.get();
  configuration.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = configuration.GetNode(supertype_id);
    assert(supertype &&
           supertype->GetNodeClass() == scada::NodeClass::VariableType);
    AddReference(configuration, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::DataType* CreateDataType(AddressSpaceImpl& configuration,
                                const scada::NodeId& id,
                                scada::QualifiedName browse_name,
                                scada::LocalizedText display_name,
                                const scada::NodeId& supertype_id) {
  assert(!configuration.GetNode(id));

  std::unique_ptr<scada::DataType> type(
      new scada::DataType(id, std::move(browse_name), std::move(display_name)));
  auto* result = type.get();
  configuration.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = configuration.GetNode(supertype_id);
    assert(supertype &&
           supertype->GetNodeClass() == scada::NodeClass::DataType);
    AddReference(configuration, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::Variable* AddProperty(AddressSpaceImpl& configuration,
                             const scada::NodeId& type_id,
                             const scada::NodeId& prop_type_id,
                             const char* browse_name,
                             const base::char16* display_name,
                             const scada::NodeId& data_type_id,
                             scada::Variant default_value = scada::Variant()) {
  auto* type = configuration.GetNode(type_id);
  assert(type && (type->GetNodeClass() == scada::NodeClass::ObjectType ||
                  type->GetNodeClass() == scada::NodeClass::VariableType));

  auto* existing_prop = configuration.GetNode(prop_type_id);
  assert(!existing_prop ||
         existing_prop->GetNodeClass() == scada::NodeClass::Variable);

  auto* result = static_cast<scada::Variable*>(existing_prop);
  if (!result) {
    auto* data_type = AsDataType(configuration.GetNode(data_type_id));
    assert(data_type);

    auto variable = std::make_unique<scada::GenericVariable>(
        prop_type_id, browse_name, display_name, *data_type);
    variable->SetValue({default_value, {}, {}, {}});
    result = variable.get();
    configuration.AddStaticNode(std::move(variable));

    AddReference(configuration, scada::id::HasTypeDefinition, prop_type_id,
                 scada::id::PropertyType);
    AddReference(configuration, scada::id::HasModellingRule, prop_type_id,
                 scada::id::ModellingRule_Mandatory);
  }

  AddReference(configuration, scada::id::HasProperty, *type, *result);
  return result;
}

scada::ReferenceType* CreateReferenceType(AddressSpaceImpl& configuration,
                                          const scada::NodeId& source_type_id,
                                          const scada::NodeId& prop_type_id,
                                          scada::QualifiedName browse_name,
                                          scada::LocalizedText display_name,
                                          const scada::NodeId& target_type_id) {
  auto* source_type = configuration.GetNode(source_type_id);
  assert(source_type &&
         (source_type->GetNodeClass() == scada::NodeClass::ObjectType ||
          source_type->GetNodeClass() == scada::NodeClass::VariableType));

  auto* target_type = configuration.GetNode(target_type_id);
  assert(target_type);

  auto* existing_prop = configuration.GetNode(prop_type_id);
  assert(!existing_prop ||
         existing_prop->GetNodeClass() == scada::NodeClass::ReferenceType);

  auto* result = static_cast<scada::ReferenceType*>(existing_prop);
  if (!result) {
    auto ref_type = std::make_unique<scada::ReferenceType>(
        prop_type_id, std::move(browse_name), std::move(display_name));
    result = ref_type.get();
    configuration.AddStaticNode(std::move(ref_type));
    AddReference(configuration, scada::id::HasSubtype,
                 scada::id::NonHierarchicalReferences, prop_type_id);
  }

  AddReference(*result, *source_type, *target_type);
  return result;
}

void AddDataVariable(AddressSpaceImpl& configuration,
                     const scada::NodeId& type_id,
                     const scada::NodeId& variable_decl_id,
                     const char* browse_name,
                     const base::char16* display_name,
                     const scada::NodeId& data_type_id,
                     scada::Variant default_value = scada::Variant()) {
  auto* type = configuration.GetNode(type_id);
  assert(type);
  assert(type->GetNodeClass() == scada::NodeClass::ObjectType ||
         type->GetNodeClass() == scada::NodeClass::VariableType);

  auto* existing_prop = configuration.GetNode(variable_decl_id);
  assert(!existing_prop ||
         existing_prop->GetNodeClass() == scada::NodeClass::Variable);

  auto* result = static_cast<scada::Variable*>(existing_prop);
  if (!result) {
    auto* variable_type = scada::AsVariableType(
        configuration.GetNode(scada::id::BaseVariableType));
    assert(variable_type);
    auto* data_type = scada::AsDataType(configuration.GetNode(data_type_id));
    assert(data_type);
    auto variable = std::make_unique<scada::GenericVariable>(
        variable_decl_id, browse_name, display_name, *data_type);
    auto now = base::Time::Now();
    variable->SetValue({std::move(default_value), {}, now, now});
    result = variable.get();
    configuration.AddStaticNode(std::move(variable));
    AddReference(configuration, scada::id::HasTypeDefinition, *result,
                 *variable_type);
    AddReference(configuration, scada::id::HasModellingRule, variable_decl_id,
                 scada::id::ModellingRule_Mandatory);
  }

  AddReference(configuration, scada::id::HasComponent, *type, *result);
}

}  // namespace

StaticAddressSpace::DeviceType::DeviceType(StandardAddressSpace& std,
                                           scada::NodeId id,
                                           scada::QualifiedName browse_name,
                                           scada::LocalizedText display_name)
    : scada::ObjectType(id, std::move(browse_name), std::move(display_name)),
      Disabled{std,          id::DeviceType_Disabled, "Disabled",
               L"Отключено", std.BoolDataType,        true},
      Online{std,      id::DeviceType_Online, "Online",
             L"Связь", std.BaseVariableType,  std.BoolDataType,
             false},
      Enabled{std,         id::DeviceType_Enabled, "Enabled",
              L"Включено", std.BaseVariableType,   std.BoolDataType,
              false} {
  SetDisplayName(std::move(display_name));
  scada::AddReference(std.HasSubtype, std.BaseObjectType, *this);
  scada::AddReference(std.HasProperty, *this, Disabled);
  scada::AddReference(std.HasComponent, *this, Online);
  scada::AddReference(std.HasComponent, *this, Enabled);
}

StaticAddressSpace::StaticAddressSpace(StandardAddressSpace& std)
    : DeviceType{std, id::DeviceType, "DeviceType", L"Устройство"} {
  scada::AddReference(std.HasSubtype, std.NonHierarchicalReference, Creates);
}

void CreateScadaAddressSpace(AddressSpaceImpl& configuration,
                             NodeFactory& node_factory) {
  node_factory.CreateNode(
      {scada::id::RootFolder,
       scada::NodeClass::Object,
       scada::id::FolderType,
       {},
       {},
       scada::NodeAttributes().set_browse_name("Root").set_display_name(
           L"Корневая папка")});

  node_factory.CreateNode({id::DataItems, scada::NodeClass::Object,
                           scada::id::FolderType, scada::id::RootFolder,
                           scada::id::Organizes,
                           scada::NodeAttributes{}
                               .set_browse_name("DataItems")
                               .set_display_name(L"Все объекты")});

  node_factory.CreateNode(
      {id::Devices, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::RootFolder, scada::id::Organizes,
       scada::NodeAttributes{}.set_browse_name("Devices").set_display_name(
           L"Все оборудование")});

  node_factory.CreateNode(
      {id::Users, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::RootFolder, scada::id::Organizes,
       scada::NodeAttributes{}.set_browse_name("Users").set_display_name(
           L"Пользователи")});

  node_factory.CreateNode({id::TsFormats, scada::NodeClass::Object,
                           scada::id::FolderType, scada::id::RootFolder,
                           scada::id::Organizes,
                           scada::NodeAttributes{}
                               .set_browse_name("TsFormats")
                               .set_display_name(L"Форматы")});

  node_factory.CreateNode({id::SimulationSignals, scada::NodeClass::Object,
                           scada::id::FolderType, scada::id::RootFolder,
                           scada::id::Organizes,
                           scada::NodeAttributes{}
                               .set_browse_name("SimulationSignals")
                               .set_display_name(L"Эмулируемые сигналы")});

  node_factory.CreateNode({id::HistoricalDatabases, scada::NodeClass::Object,
                           scada::id::FolderType, scada::id::RootFolder,
                           scada::id::Organizes,
                           scada::NodeAttributes{}
                               .set_browse_name("HistoricalDatabases")
                               .set_display_name(L"Базы данных")});

  node_factory.CreateNode({id::TransmissionItems, scada::NodeClass::Object,
                           scada::id::FolderType, scada::id::RootFolder,
                           scada::id::Organizes,
                           scada::NodeAttributes{}
                               .set_browse_name("TransmissionItems")
                               .set_display_name(L"Ретрансляция")});

  // Link
  {
    CreateObjectType(configuration, id::LinkType, "LinkType", L"Направление",
                     id::DeviceType);
    AddReference(configuration, id::Creates, id::Devices, id::LinkType);
    AddProperty(configuration, id::LinkType, id::LinkType_Transport,
                "TransportString", L"Транспорт", scada::id::String);
    AddDataVariable(configuration, id::LinkType, id::LinkType_ConnectCount,
                    "ConnectCount", L"ConnectCount", scada::id::Int32, 0);
    AddDataVariable(configuration, id::LinkType, id::LinkType_ActiveConnections,
                    "ActiveConnections", L"ActiveConnections", scada::id::Int32,
                    0);
    AddDataVariable(configuration, id::LinkType, id::LinkType_MessagesOut,
                    "MessagesOut", L"MessagesOut", scada::id::Int32, 0);
    AddDataVariable(configuration, id::LinkType, id::LinkType_MessagesIn, "MessagesIn",
                    L"MessagesIn", scada::id::Int32, 0);
    AddDataVariable(configuration, id::LinkType, id::LinkType_BytesOut, "BytesOut",
                    L"BytesOut", scada::id::Int32, 0);
    AddDataVariable(configuration, id::LinkType, id::LinkType_BytesIn, "BytesIn",
                    L"BytesIn", scada::id::Int32, 0);
  }

  // Simulation Item
  {
    CreateVariableType(configuration, id::SimulationSignalType,
                       "SimulationSignalType", L"Эмулируемый сигнал",
                       scada::id::BaseDataType, scada::id::BaseVariableType);
    AddReference(configuration, id::Creates, id::SimulationSignals,
                 id::SimulationSignalType);
    AddProperty(configuration, id::SimulationSignalType,
                id::SimulationSignalType_Type, "Type", L"Тип",
                scada::id::Int32, 0);
    AddProperty(configuration, id::SimulationSignalType,
                id::SimulationSignalType_Period, "Period", L"Период (мс)",
                scada::id::Int32, 60000);
    AddProperty(configuration, id::SimulationSignalType,
                id::SimulationSignalType_Phase, "Phase", L"Фаза (мс)",
                scada::id::Int32, 0);
    AddProperty(configuration, id::SimulationSignalType,
                id::SimulationSignalType_UpdateInterval, "UpdateInterval",
                L"Обновление (мс)", scada::id::Int32, 1000);
  }

  // Historical DB
  {
    CreateObjectType(configuration, id::HistoricalDatabaseType,
                     "HistoricalDatabaseType", L"База данных",
                     scada::id::BaseObjectType);
    AddReference(configuration, id::Creates, id::HistoricalDatabases,
                 id::HistoricalDatabaseType);
    AddProperty(configuration, id::HistoricalDatabaseType,
                id::HistoricalDatabaseType_Depth, "Depth", L"Глубина (дн)",
                scada::id::Int32, 1);
    AddDataVariable(configuration, id::HistoricalDatabaseType,
                    id::HistoricalDatabaseType_WriteValueDuration, "WriteValueDuration",
                    L"WriteValueDuration", scada::id::Int32);
    AddDataVariable(configuration, id::HistoricalDatabaseType,
                    id::HistoricalDatabaseType_PendingTaskCount, "PendingTaskCount",
                    L"PendingTaskCount", scada::id::Int32);
    AddDataVariable(configuration, id::HistoricalDatabaseType,
                    id::HistoricalDatabaseType_EventCleanupDuration, "EventCleanupDuration",
                    L"EventCleanupDuration", scada::id::Int32);
    AddDataVariable(configuration, id::HistoricalDatabaseType,
                    id::HistoricalDatabaseType_ValueCleanupDuration, "ValueCleanupDuration",
                    L"ValueCleanupDuration", scada::id::Int32);
  }

  // System historical database.
  node_factory.CreateNode(
      {id::SystemDatabase,
       scada::NodeClass::Object,
       id::HistoricalDatabaseType,
       id::HistoricalDatabases,
       scada::id::Organizes,
       scada::NodeAttributes().set_browse_name("System").set_display_name(
           L"Системная база данных"),
       {{id::HistoricalDatabaseType_Depth, 30}}});

  // TsFormat
  {
    CreateObjectType(configuration, id::TsFormatType, "TsFormatType", L"Формат",
                     scada::id::BaseObjectType);
    AddReference(configuration, id::Creates, id::TsFormats, id::TsFormatType);
    AddProperty(configuration, id::TsFormatType, id::TsFormatType_OpenLabel,
                "OpenLabel", L"Подпись 0", scada::id::LocalizedText);
    AddProperty(configuration, id::TsFormatType, id::TsFormatType_CloseLabel,
                "CloseLabel", L"Подпись 1", scada::id::LocalizedText);
    AddProperty(configuration, id::TsFormatType, id::TsFormatType_OpenColor,
                "OpenColor", L"Цвет 0", scada::id::Int32);
    AddProperty(configuration, id::TsFormatType, id::TsFormatType_CloseColor,
                "CloseColor", L"Цвет 1", scada::id::Int32);
  }

  // DataGroup
  {
    CreateObjectType(configuration, id::DataGroupType, "DataGroupType",
                     L"Группа", scada::id::BaseObjectType);
    AddReference(configuration, id::Creates, id::DataItems, id::DataGroupType);
    AddReference(configuration, id::Creates, id::DataGroupType,
                 id::DataGroupType);  // recursive
    AddProperty(configuration, id::DataGroupType, id::DataGroupType_Simulated,
                "Simulated", L"Эмуляция", scada::id::Boolean, false);
  }

  // DataItem
  {
    CreateVariableType(configuration, id::DataItemType, "DataItemType",
                       L"Объект", scada::id::BaseDataType,
                       scada::id::BaseVariableType);
    AddReference(configuration, id::Creates, id::DataGroupType,
                 id::DataItemType);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Alias,
                "Alias", L"Алиас", scada::id::String);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Severity,
                "Severity", L"Важность", scada::id::Int32, 1);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Input1,
                "Input1", L"Ввод1", scada::id::String);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Input2,
                "Input2", L"Ввод2", scada::id::String);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Output,
                "Output", L"Вывод", scada::id::String);
    AddProperty(configuration, id::DataItemType,
                id::DataItemType_OutputCondition, "OutputCondition",
                L"Условие (Управление)", scada::id::String);
    AddProperty(configuration, id::DataItemType, id::DataItemType_StalePeriod,
                "StalePeriod", L"Устаревание (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Simulated,
                "Simulated", L"Эмуляция", scada::id::Boolean, false);
    CreateReferenceType(configuration, id::DataItemType,
                        id::HasSimulationSignal, "HasSimulationSignal",
                        L"Эмулируемый сигнал", id::SimulationSignalType);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Locked,
                "Locked", L"Блокирован", scada::id::Boolean, false);
    CreateReferenceType(configuration, id::DataItemType,
                        id::HasHistoricalDatabase, "ObjectHistoricalDb",
                        L"База данных", id::HistoricalDatabaseType);
  }

  // DiscreteItem
  {
    CreateVariableType(configuration, id::DiscreteItemType, "DiscreteItemType",
                       L"Объект ТС", scada::id::BaseDataType, id::DataItemType);
    AddProperty(configuration, id::DiscreteItemType, id::DiscreteItemType_Inversion,
                "Inverted", L"Инверсия", scada::id::Boolean, false);
    CreateReferenceType(configuration, id::DiscreteItemType, id::HasTsFormat,
                        "HasTsFormat", L"Параметры", id::TsFormatType);
  }

  // AnalogItem
  {
    CreateVariableType(configuration, id::AnalogItemType, "AnalogItemType",
                       L"Объект ТИТ", scada::id::BaseDataType,
                       id::DataItemType);
    AddProperty(configuration, id::AnalogItemType,
                id::AnalogItemType_DisplayFormat, "DisplayFormat", L"Формат",
                scada::id::String, "0.####");
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_Conversion,
                "ConversionType", L"Преобразование", scada::id::Int32, 0);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_Clamping,
                "ClampingType", L"Ограничение диапазона", scada::id::Boolean,
                false);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_EuLo, "EuLo",
                L"Лог мин", scada::id::Double, -100);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_EuHi, "EuHi",
                L"Лог макс", scada::id::Double, 100);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_IrLo, "IrLo",
                L"Физ мин", scada::id::Double, 0);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_IrHi, "IrHi",
                L"Физ макс", scada::id::Double, 65535);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_LimitLo,
                "LimitLo", L"Уставка нижняя предаварийная", scada::id::Double);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_LimitHi,
                "LimitHi", L"Уставка верхняя предаварийная", scada::id::Double);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_LimitLoLo,
                "LimitLoLo", L"Уставка нижняя аварийная", scada::id::Double);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_LimitHiHi,
                "LimitHiHi", L"Уставка верхняя аварийная", scada::id::Double);
    AddProperty(configuration, id::AnalogItemType,
                id::AnalogItemType_EngineeringUnits, "EngineeringUnits",
                L"Единицы измерения", scada::id::LocalizedText);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_Aperture,
                "Aperture", L"Апертура", scada::id::Double, 0.0);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_Deadband,
                "Deadband", L"Мертвая зона", scada::id::Double, 0.0);
  }

  // Modbus Port Device
  {
    CreateObjectType(configuration, id::ModbusLinkType, "ModbusLinkType",
                     L"Направление MODBUS", id::LinkType);
    auto* mode_type = CreateDataType(
        configuration, id::ModbusLinkType_Mode, "ModbusPortModeType",
        L"Тип направления MODBUS", scada::id::Int32);
    mode_type->enum_strings = {L"Опрос", L"Ретрансляция", L"Прослушка"};
    AddProperty(configuration, id::ModbusLinkType, id::ModbusLinkType_Protocol,
                "Mode", L"Режим", id::ModbusLinkType_Mode, 0);
  }

  // Modbus Device
  {
    CreateObjectType(configuration, id::ModbusDeviceType, "ModbusDeviceType",
                     L"Устройство MODBUS", id::DeviceType);
    AddReference(configuration, id::Creates, id::ModbusLinkType,
                 id::ModbusDeviceType);
    AddProperty(configuration, id::ModbusDeviceType,
                id::ModbusDeviceType_Address, "Address", L"Адрес",
                scada::id::Int32, 1);
    AddProperty(configuration, id::ModbusDeviceType,
                id::ModbusDeviceType_SendRetryCount, "RepeatCount",
                L"Попыток повторов передачи", scada::id::Int32, 3);
  }

  // User
  {
    CreateObjectType(configuration, id::UserType, "UserType", L"Пользователь",
                     scada::id::BaseObjectType);
    AddReference(configuration, id::Creates, id::Users, id::UserType);
    AddProperty(configuration, id::UserType, id::UserType_AccessRights,
                "AccessRights", L"Права", scada::id::Int32, 0);
  }

  // Root user.
  int root_privileges = (1 << static_cast<int>(scada::Privilege::Configure)) |
                        (1 << static_cast<int>(scada::Privilege::Control));
  node_factory.CreateNode({id::RootUser,
                           scada::NodeClass::Object,
                           id::UserType,
                           id::Users,
                           scada::id::Organizes,
                           scada::NodeAttributes().set_display_name(L"root"),
                           {{id::UserType_AccessRights, root_privileges}}});

  // IEC Link Device
  {
    auto* type = CreateObjectType(configuration, id::Iec60870LinkType,
                                  "Iec60870LinkType", L"Направление МЭК-60870",
                                  id::LinkType);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_Protocol, "Protocol", L"Протокол",
                scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_Mode, "Mode", L"Режим", scada::id::Int32,
                0);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_SendQueueSize, "SendQueueSize",
                L"Очередь передачи (K)", scada::id::Int32, 12);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_ReceiveQueueSize, "ReceiveQueueSize",
                L"Очередь приема (W)", scada::id::Int32, 8);
    AddProperty(configuration, id::Iec60870LinkType, id::Iec60870LinkType_ConnectTimeout,
                "ConnectTimeout", L"Таймаут связи (T0, с)", scada::id::Int32,
                30);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_ConfirmationTimeout,
                "ConfirmationTimeout", L"Таймаут подтверждения (с)",
                scada::id::Int32, 5);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_TerminationTimeout,
                "TerminationTimeout", L"Таймаут операции (с)", scada::id::Int32,
                20);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_DeviceAddressSize, "DeviceAddressSize",
                L"Размер адреса устройства", scada::id::Int32, 2);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_COTSize, "CotSize",
                L"Размер причины передачи", scada::id::Int32, 2);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_InfoAddressSize, "InfoAddressSize",
                L"Размер адреса объекта", scada::id::Int32, 3);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_DataCollection, "CollectData",
                L"Сбор данных", scada::id::Boolean, true);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_SendRetryCount, "SendRetries",
                L"Попыток повторов передачи", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_CRCProtection, "CrcProtection",
                L"Защита CRC", scada::id::Boolean, false);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_SendTimeout, "SendTimeout",
                L"Таймаут передачи (T1, с)", scada::id::Int32, 5);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_ReceiveTimeout, "ReceiveTimeout",
                L"Таймаут приема (T2, с)", scada::id::Int32, 10);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_IdleTimeout, "IdleTimeout",
                L"Таймаут простоя (T3, с)", scada::id::Int32, 30);
    AddProperty(configuration, id::Iec60870LinkType,
                id::Iec60870LinkType_AnonymousMode, "AnonymousMode",
                L"Анонимный режим", scada::id::Boolean);
  }

  // IEC Device
  {
    CreateObjectType(configuration, id::Iec60870DeviceType,
                     "Iec60870DeviceType", L"Устройство МЭК-60870",
                     id::DeviceType);
    AddReference(configuration, id::Creates, id::Iec60870LinkType,
                 id::Iec60870DeviceType);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_Address, "Address", L"Адрес",
                scada::id::Int32, 1);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_LinkAddress, "LinkAddress",
                L"Адрес коммутатора", scada::id::Int32, 1);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_StartupInterrogation,
                "InterrogateOnStart", L"Полный опрос при запуске",
                scada::id::Boolean, true);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriod,
                "InterrogationPeriod", L"Период полного опроса (с)",
                scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_StartupClockSync,
                "SynchronizeClockOnStart", L"Синхронизация часов при запуске",
                scada::id::Boolean, false);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_ClockSyncPeriod,
                "ClockSynchronizationPeriod", L"Период синхронизации часов (с)",
                scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_UtcTime, "UtcTime", L"Время UTC",
                scada::id::Boolean, false);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup1, "Group1PollPeriod",
                L"Период опроса группы 1 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup2, "Group1PollPeriod",
                L"Период опроса группы 2 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup3, "Group1PollPeriod",
                L"Период опроса группы 3 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup4, "Group1PollPeriod",
                L"Период опроса группы 4 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup5, "Group1PollPeriod",
                L"Период опроса группы 5 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup6, "Group1PollPeriod",
                L"Период опроса группы 6 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup7, "Group1PollPeriod",
                L"Период опроса группы 7 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup8, "Group1PollPeriod",
                L"Период опроса группы 8 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup9, "Group1PollPeriod",
                L"Период опроса группы 9 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup10, "Group1PollPeriod",
                L"Период опроса группы 10 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup11, "Group1PollPeriod",
                L"Период опроса группы 11 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup12, "Group1PollPeriod",
                L"Период опроса группы 12 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup13, "Group1PollPeriod",
                L"Период опроса группы 13 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup14, "Group1PollPeriod",
                L"Период опроса группы 14 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup15, "Group1PollPeriod",
                L"Период опроса группы 15 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType,
                id::Iec60870DeviceType_InterrogationPeriodGroup16, "Group1PollPeriod",
                L"Период опроса группы 16 (с)", scada::id::Int32, 0);
  }

  // IEC-61850 Logical Node
  {
    CreateObjectType(configuration, id::Iec61850LogicalNodeType,
                     "Iec61850LogicalNodeType", L"Логический узел МЭК-61850",
                     scada::id::BaseObjectType);
  }

  // IEC-61850 Data Variable
  {
    CreateVariableType(configuration, id::Iec61850DataVariableType,
                       "Iec61850DataVariableType", L"Объект данных МЭК-61850",
                       scada::id::BaseDataType, scada::id::BaseVariableType);
  }

  // IEC-61850 Control Object
  {
    CreateObjectType(configuration, id::Iec61850ControlObjectType,
                     "Iec61850ControlObjectType",
                     L"Объект управления МЭК-61850", scada::id::BaseObjectType);
  }

  // IEC-61850 Device
  {
    CreateObjectType(configuration, id::Iec61850DeviceType,
                     "Iec61850DeviceType", L"Устройство МЭК-61850",
                     id::DeviceType);
    AddReference(configuration, id::Creates, id::Devices,
                 id::Iec61850DeviceType);
    {
      auto model = std::make_unique<scada::GenericObject>();
      model->set_id(id::Iec61850DeviceType_Model);
      model->SetBrowseName("Model");
      model->SetDisplayName(L"Модель");
      configuration.AddStaticNode(std::move(model));
      AddReference(configuration, scada::id::HasTypeDefinition,
                   id::Iec61850DeviceType_Model, id::Iec61850LogicalNodeType);
      AddReference(configuration, scada::id::HasModellingRule,
                   id::Iec61850DeviceType_Model,
                   scada::id::ModellingRule_Mandatory);
      // Only HasComponent can be nested now.
      AddReference(configuration, scada::id::HasComponent,
                   id::Iec61850DeviceType, id::Iec61850DeviceType_Model);
    }
    AddProperty(configuration, id::Iec61850DeviceType,
                id::Iec61850DeviceType_Host, "Host", L"Хост",
                scada::id::String);
    AddProperty(configuration, id::Iec61850DeviceType,
                id::Iec61850DeviceType_Port, "Port", L"Порт", scada::id::Int32,
                102);
  }

  // IEC-61850 ConfigurableObject
  {
    CreateObjectType(configuration, id::Iec61850ConfigurableObjectType,
                     "ConfigurableObject", L"Параметр МЭК-61850",
                     scada::id::BaseObjectType);
    AddReference(configuration, id::Creates, id::Iec61850ConfigurableObjectType,
                 id::Iec61850ConfigurableObjectType);
    AddProperty(configuration, id::Iec61850ConfigurableObjectType,
                id::Iec61850ConfigurableObjectType_Reference, "Address",
                L"Адрес", scada::id::String);
  }

  // IEC-61850 RCB
  {
    CreateObjectType(configuration, id::Iec61850RcbType, "RCB",
                     L"RCB МЭК-61850", id::Iec61850ConfigurableObjectType);
    AddReference(configuration, id::Creates, id::Iec61850DeviceType,
                 id::Iec61850RcbType);
  }

  // IEC Retransmission Item
  {
    CreateObjectType(configuration, id::TransmissionItemType,
                     "TransmissionItemType", L"Ретрансляция",
                     scada::id::BaseObjectType);
    AddReference(configuration, id::Creates, id::TransmissionItems,
                 id::TransmissionItemType);
    CreateReferenceType(configuration, id::TransmissionItemType,
                        id::HasTransmissionSource, "IecTransmitSource",
                        L"Объект источник", scada::id::BaseVariableType);
    CreateReferenceType(configuration, id::TransmissionItemType,
                        id::HasTransmissionTarget,
                        "IecTransmitTargetDevice", L"Устройство приемник",
                        id::DeviceType);
    AddProperty(configuration, id::TransmissionItemType,
                id::TransmissionItemType_SourceAddress, "InfoAddress",
                L"Адрес объекта приемника", scada::id::Int32, 1);
  }
}
