#include "core/types.h"

#include "core/configuration_impl.h"
#include "common/scada_node_ids.h"
#include "core/node_utils.h"
#include "core/object.h"
#include "core/type_definition.h"
#include "core/variable.h"

const TreeDef kTreeDefs[] = {
    { id::DataItems, kObjectsRootTypeId, L"Все объекты" },
    { id::Devices, kDevicesRootTypeId, L"Все оборудование" },
    { id::Users, kUsersRootTypeId, L"Пользователи" },
    { id::TsFormats, kTsFormatRootTypeId, L"Форматы" },
    { id::SimulationSignals, kSimulatorRootTypeId, L"Эмулируемые сигналы" },
    { id::HistoricalDatabases, kHistoricalDbRootTypeId, L"Базы данных" },
    { id::TransmissionItems, kIecTransmitRootTypeId, L"Ретрансляция" },
};

GenericDataVariable::GenericDataVariable(StandardAddressSpace& std, scada::NodeId id, scada::QualifiedName browse_name,
    scada::LocalizedText display_name, scada::VariableType& variable_type, const scada::DataType& data_type,
    scada::Variant default_value)
    : GenericVariable(std::move(id), std::move(browse_name), std::move(display_name), data_type, std::move(default_value)) {
  scada::AddReference(std.HasTypeDefinition, *this, variable_type);
  scada::AddReference(std.HasModellingRule, *this, std.ModellingRule_Mandatory);
}

GenericProperty::GenericProperty(StandardAddressSpace& std, scada::NodeId id, scada::QualifiedName browse_name,
    scada::LocalizedText display_name, const scada::DataType& data_type, scada::Variant default_value)
    : GenericVariable(std::move(id), std::move(browse_name), std::move(display_name), data_type, std::move(default_value)) {
  scada::AddReference(std.HasTypeDefinition, *this, std.PropertyType);
  scada::AddReference(std.HasModellingRule, *this, std.ModellingRule_Mandatory);
}

StaticAddressSpace::DeviceType::DeviceType(StandardAddressSpace& std, scada::NodeId id, scada::QualifiedName browse_name,
    scada::LocalizedText display_name)
    : scada::ObjectType(id, std::move(browse_name), std::move(display_name)),
      Disabled{std, id::DeviceType_Disabled, "Disabled", L"Отключено", std.BoolDataType, true},
      Online{std, id::DeviceType_Online, "Online", L"Связь", std.BaseVariableType, std.BoolDataType, false},
      Enabled{std, id::DeviceType_Enabled, "Enabled", L"Включено", std.BaseVariableType, std.BoolDataType, false} {
  SetDisplayName(std::move(display_name));
  scada::AddReference(std.HasSubtype, std.BaseObjectType, *this);
  scada::AddReference(std.HasProperty, *this, Disabled);
  scada::AddReference(std.HasComponent, *this, Online);
  scada::AddReference(std.HasComponent, *this, Enabled);
}

scada::ObjectType* CreateObjectType(ConfigurationImpl& configuration,
    const scada::NodeId& id, scada::QualifiedName qualified_name, scada::LocalizedText display_name, const scada::NodeId& supertype_id) {
  assert(!configuration.GetNode(id));

  auto type = std::make_unique<scada::ObjectType>(id, std::move(qualified_name), std::move(display_name));
  auto* result = type.get();
  configuration.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = configuration.GetNode(supertype_id);
    assert(supertype && supertype->GetNodeClass() == scada::NodeClass::ObjectType);
    AddReference(configuration, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::VariableType* CreateVariableType(ConfigurationImpl& configuration, const scada::NodeId& id,
    scada::QualifiedName browse_name, scada::LocalizedText display_name, const scada::NodeId& data_type_id, const scada::NodeId& supertype_id) {
  assert(!configuration.GetNode(id));

  auto* data_type = configuration.GetNode(data_type_id);
  assert(data_type && data_type->GetNodeClass() == scada::NodeClass::DataType);

  auto type = std::make_unique<scada::VariableType>(id, std::move(browse_name), std::move(display_name), *static_cast<const scada::DataType*>(data_type));
  auto* result = type.get();
  configuration.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = configuration.GetNode(supertype_id);
    assert(supertype && supertype->GetNodeClass() == scada::NodeClass::VariableType);
    AddReference(configuration, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::DataType* CreateDataType(ConfigurationImpl& configuration, const scada::NodeId& id,
    scada::QualifiedName browse_name, scada::LocalizedText display_name, const scada::NodeId& supertype_id) {
  assert(!configuration.GetNode(id));

  std::unique_ptr<scada::DataType> type(new scada::DataType(id, std::move(browse_name), std::move(display_name)));
  auto* result = type.get();
  configuration.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = configuration.GetNode(supertype_id);
    assert(supertype && supertype->GetNodeClass() == scada::NodeClass::DataType);
    AddReference(configuration, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::Variable* AddProperty(ConfigurationImpl& configuration,
    const scada::NodeId& type_id, const scada::NodeId& prop_type_id,
    const char* browse_name, const base::char16* display_name,
    const scada::NodeId& data_type_id, scada::Variant default_value = scada::Variant()) {
  auto* type = configuration.GetNode(type_id);
  assert(type && (type->GetNodeClass() == scada::NodeClass::ObjectType ||
                  type->GetNodeClass() == scada::NodeClass::VariableType));

  auto* existing_prop = configuration.GetNode(prop_type_id);
  assert(!existing_prop || existing_prop->GetNodeClass() == scada::NodeClass::Variable);

  auto* result = static_cast<scada::Variable*>(existing_prop);
  if (!result) {
    auto* data_type = AsDataType(configuration.GetNode(data_type_id));
    assert(data_type);

    auto variable = std::make_unique<scada::GenericVariable>(prop_type_id, browse_name, display_name, *data_type);
    variable->SetValue({ default_value, {}, {}, {} });
    result = variable.get();
    configuration.AddStaticNode(std::move(variable));

    AddReference(configuration, scada::id::HasTypeDefinition, prop_type_id, scada::id::PropertyType);
    AddReference(configuration, scada::id::HasModellingRule, prop_type_id, scada::id::ModellingRule_Mandatory);
  }

  AddReference(configuration, scada::id::HasProperty, *type, *result);
  return result;
}

scada::ReferenceType* CreateReferenceType(ConfigurationImpl& configuration,
    const scada::NodeId& source_type_id, const scada::NodeId& prop_type_id, scada::QualifiedName browse_name,
    scada::LocalizedText display_name, const scada::NodeId& target_type_id) {
  auto* source_type = configuration.GetNode(source_type_id);
  assert(source_type && (source_type->GetNodeClass() == scada::NodeClass::ObjectType ||
                         source_type->GetNodeClass() == scada::NodeClass::VariableType));

  auto* target_type = configuration.GetNode(target_type_id);
  assert(target_type);

  auto* existing_prop = configuration.GetNode(prop_type_id);
  assert(!existing_prop || existing_prop->GetNodeClass() == scada::NodeClass::ReferenceType);

  auto* result = static_cast<scada::ReferenceType*>(existing_prop);
  if (!result) {
    auto ref_type = std::make_unique<scada::ReferenceType>(prop_type_id, std::move(browse_name), std::move(display_name));
    result = ref_type.get();
    configuration.AddStaticNode(std::move(ref_type));
    AddReference(configuration, scada::id::HasSubtype, scada::id::NonHierarchicalReferences, prop_type_id);
  }

  AddReference(*result, *source_type, *target_type);
  return result;
}

void AddDataVariable(ConfigurationImpl& configuration,
    const scada::NodeId& type_id, const scada::NodeId& variable_decl_id, const char* browse_name,
    const base::char16* display_name, const scada::NodeId& data_type_id,
    scada::Variant default_value = scada::Variant()) {
  auto* type = configuration.GetNode(type_id);
  assert(type);
  assert(type->GetNodeClass() == scada::NodeClass::ObjectType ||
         type->GetNodeClass() == scada::NodeClass::VariableType);

  auto* existing_prop = configuration.GetNode(variable_decl_id);
  assert(!existing_prop || existing_prop->GetNodeClass() == scada::NodeClass::Variable);

  auto* result = static_cast<scada::Variable*>(existing_prop);
  if (!result) {
    auto* variable_type = scada::AsVariableType(configuration.GetNode(scada::id::BaseVariableType));
    assert(variable_type);
    auto* data_type = scada::AsDataType(configuration.GetNode(data_type_id));
    assert(data_type);
    auto variable = std::make_unique<scada::GenericVariable>(variable_decl_id, browse_name, display_name, *data_type);
    auto now = base::Time::Now();
    variable->SetValue({ std::move(default_value), {}, now, now });
    result = variable.get();
    configuration.AddStaticNode(std::move(variable));
    AddReference(configuration, scada::id::HasTypeDefinition, *result, *variable_type);
    AddReference(configuration, scada::id::HasModellingRule, variable_decl_id, scada::id::ModellingRule_Mandatory);
  }

  AddReference(configuration, scada::id::HasComponent, *type, *result);
}

void AddComponent(ConfigurationImpl& configuration, const scada::NodeId& parent_type_id, const scada::NodeId& component_id,
                  const scada::NodeId& component_type_id) {
  auto* parent_type = scada::AsTypeDefinition(configuration.GetNode(parent_type_id));
  assert(parent_type && (parent_type->GetNodeClass() == scada::NodeClass::ObjectType ||
                         parent_type->GetNodeClass() == scada::NodeClass::VariableType));

  auto* component_type = scada::AsTypeDefinition(configuration.GetNode(component_type_id));
  assert(component_type && component_type->GetNodeClass() == scada::NodeClass::ObjectType ||
                           component_type->GetNodeClass() == scada::NodeClass::VariableType);

  auto* component_decl = configuration.GetNode(component_id);
  assert(!component_decl || component_decl->GetNodeClass() == scada::NodeClass::Object ||
                            component_decl->GetNodeClass() == scada::NodeClass::Variable);
  if (!component_decl) {
    if (component_type->GetNodeClass() == scada::NodeClass::ObjectType) {
      auto decl = std::make_unique<scada::GenericObject>();
      decl->set_id(component_id);
      component_decl = decl.get();
      configuration.AddStaticNode(std::move(decl));
    } else {
      auto* variable_type = scada::AsVariableType(component_type);
      assert(variable_type);
      // TODO: Name
      auto decl = std::make_unique<scada::GenericVariable>(component_id, scada::QualifiedName{}, scada::LocalizedText{}, variable_type->data_type());
      component_decl = decl.get();
      configuration.AddStaticNode(std::move(decl));
    }

    AddReference(configuration, scada::id::HasTypeDefinition, *component_decl, *component_type);
  }

  AddReference(configuration, scada::id::HasComponent, *parent_type, *component_decl);
}

StandardAddressSpace::StandardAddressSpace() {
  scada::AddReference(HasSubtype, HierarchicalReference, Aggregates);
  scada::AddReference(HasSubtype, HierarchicalReference, Organizes);
  scada::AddReference(HasSubtype, Aggregates, HasProperty);
  scada::AddReference(HasSubtype, Aggregates, HasComponent);
  scada::AddReference(HasSubtype, Aggregates, HasSubtype);
  scada::AddReference(HasSubtype, NonHierarchicalReference, HasTypeDefinition);
  scada::AddReference(HasSubtype, NonHierarchicalReference, HasModellingRule);

  scada::AddReference(HasSubtype, BoolDataType, BaseDataType);
  scada::AddReference(HasSubtype, IntDataType, BaseDataType);
  scada::AddReference(HasSubtype, DoubleDataType, BaseDataType);
  scada::AddReference(HasSubtype, StringDataType, BaseDataType);
  scada::AddReference(HasSubtype, LocalizedTextDataType, BaseDataType);
  scada::AddReference(HasSubtype, NodeIdDataType, BaseDataType);

  scada::AddReference(HasSubtype, ModellingRule_Mandatory, BaseObjectType);
  scada::AddReference(HasSubtype, FolderType, BaseObjectType);

  scada::AddReference(HasSubtype, BaseVariableType, PropertyType);
}

StaticAddressSpace::StaticAddressSpace(StandardAddressSpace& std)
    : DeviceType{std, id::DeviceType, "DeviceType", L"Устройство"} {
}

void CreateCommonTypes(ConfigurationImpl& configuration) {
  static_assert(_countof(kTreeDefs) == kTreeCount, "Not all trees are described");

  // Root folders
  CreateObjectType(configuration, kObjectsRootTypeId, "DataItems", L"Все объекты", scada::id::BaseObjectType);
  CreateObjectType(configuration, kDevicesRootTypeId, "Devices", L"Все оборудование", scada::id::BaseObjectType);
  CreateObjectType(configuration, kUsersRootTypeId, "Users", L"Пользователи", scada::id::BaseObjectType);
  CreateObjectType(configuration, kTsFormatRootTypeId, "TsFormats", L"Форматы", scada::id::BaseObjectType);
  CreateObjectType(configuration, kSimulatorRootTypeId, "SimulationSignals", L"Эмулируемые сигналы", scada::id::BaseObjectType);
  CreateObjectType(configuration, kHistoricalDbRootTypeId, "HistoricalDatabases", L"Базы данных", scada::id::BaseObjectType);
  CreateObjectType(configuration, kIecTransmitRootTypeId, "TransmissionItems", L"Ретрансляция", scada::id::BaseObjectType);

  // Link
  {
    CreateObjectType(configuration, kLinkTypeId, "LinkType", L"Направление", id::DeviceType);
    AddComponent(configuration, kDevicesRootTypeId, kDevicesRoot_Links, kLinkTypeId);
    AddProperty(configuration, kLinkTypeId, kLinkTransportStringPropTypeId, "TransportString", L"Транспорт", scada::id::String);
    AddDataVariable(configuration, kLinkTypeId, kLink_ConnectCount, "ConnectCount", L"ConnectCount", scada::id::Int32, 0);
    AddDataVariable(configuration, kLinkTypeId, kLink_ActiveConnections, "ActiveConnections", L"ActiveConnections", scada::id::Int32, 0);
    AddDataVariable(configuration, kLinkTypeId, kLink_MessagesOut, "MessagesOut", L"MessagesOut", scada::id::Int32, 0);
    AddDataVariable(configuration, kLinkTypeId, kLink_MessagesIn, "MessagesIn", L"MessagesIn", scada::id::Int32, 0);
    AddDataVariable(configuration, kLinkTypeId, kLink_BytesOut, "BytesOut", L"BytesOut", scada::id::Int32, 0);
    AddDataVariable(configuration, kLinkTypeId, kLink_BytesIn, "BytesIn", L"BytesIn", scada::id::Int32, 0);
  }

  // Simulation Item
  {
    CreateVariableType(configuration, id::SimulationSignalType, "SimulationSignalType", L"Эмулируемый сигнал", scada::id::BaseDataType, scada::id::BaseVariableType);
    AddComponent(configuration, kSimulatorRootTypeId, kSimulatorRoot_Signals, id::SimulationSignalType);
    AddProperty(configuration, id::SimulationSignalType, kSimulationSignalTypePropTypeId, "Type", L"Тип", scada::id::Int32, 0);
    AddProperty(configuration, id::SimulationSignalType, kSimulationSignalPeriodPropTypeId, "Period", L"Период (мс)", scada::id::Int32, 60000);
    AddProperty(configuration, id::SimulationSignalType, kSimulationSignalPhasePropTypeId, "Phase", L"Фаза (мс)", scada::id::Int32, 0);
    AddProperty(configuration, id::SimulationSignalType, kSimulationSignalUpdateIntervalPropTypeId, "UpdateInterval", L"Обновление (мс)", scada::id::Int32, 1000);
  }

  // Historical DB
  {
    CreateObjectType(configuration, id::HistoricalDatabaseType, "HistoricalDatabaseType", L"База данных", scada::id::BaseObjectType);
    AddComponent(configuration, kHistoricalDbRootTypeId, kHistoricalDbRoot_Databases, id::HistoricalDatabaseType);
    AddProperty(configuration, id::HistoricalDatabaseType, kHistoricalDbDepthPropTypeId, "Depth", L"Глубина (дн)", scada::id::Int32, 1);
    AddDataVariable(configuration, id::HistoricalDatabaseType, kHistoricalDb_WriteValueDuration, "WriteValueDuration", L"WriteValueDuration", scada::id::Int32);
    AddDataVariable(configuration, id::HistoricalDatabaseType, kHistoricalDb_PendingTaskCount, "PendingTaskCount", L"PendingTaskCount", scada::id::Int32);
    AddDataVariable(configuration, id::HistoricalDatabaseType, kHistoricalDb_EventCleanupDuration, "EventCleanupDuration", L"EventCleanupDuration", scada::id::Int32);
    AddDataVariable(configuration, id::HistoricalDatabaseType, kHistoricalDb_ValueCleanupDuration, "ValueCleanupDuration", L"ValueCleanupDuration", scada::id::Int32);
  }

  // TsFormat
  {
    CreateObjectType(configuration, id::TsFormatType, "TsFormatType", L"Формат", scada::id::BaseObjectType);
    AddComponent(configuration, kTsFormatRootTypeId, kTsFormatRoot_Formats, id::TsFormatType);
    AddProperty(configuration, id::TsFormatType, id::TsFormatType_OpenLabel, "OpenLabel", L"Подпись 0", scada::id::LocalizedText);
    AddProperty(configuration, id::TsFormatType, id::TsFormatType_CloseLabel, "CloseLabel", L"Подпись 1", scada::id::LocalizedText);
    AddProperty(configuration, id::TsFormatType, kTsFormatOpenColorPropTypeId, "OpenColor", L"Цвет 0", scada::id::Int32);
    AddProperty(configuration, id::TsFormatType, kTsFormatCloseColorPropTypeId, "CloseColor", L"Цвет 1", scada::id::Int32);
  }

  // DataGroup
  {
    CreateObjectType(configuration, id::DataGroupType, "DataGroupType", L"Группа", scada::id::BaseObjectType);
    AddComponent(configuration, kObjectsRootTypeId, kObjectsRoot_Folders, id::DataGroupType);
    AddComponent(configuration, id::DataGroupType, kFolder_Subfolders, id::DataGroupType); // recursive
    AddProperty(configuration, id::DataGroupType, id::DataGroupType_Simulated, "Simulated", L"Эмуляция", scada::id::Boolean, false);
  }

  // DataItem
  {
    CreateVariableType(configuration, id::DataItemType, "DataItemType", L"Объект", scada::id::BaseDataType, scada::id::BaseVariableType);
    AddComponent(configuration, id::DataGroupType, kFolder_DataItems, id::DataItemType);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Alias, "Alias", L"Алиас", scada::id::String);
    AddProperty(configuration, id::DataItemType, kObjectSeverityPropTypeId, "Severity", L"Важность", scada::id::Int32, 1);
    AddProperty(configuration, id::DataItemType, kObjectInput1PropTypeId, "Input1", L"Ввод1", scada::id::String);
    AddProperty(configuration, id::DataItemType, kObjectInput2PropTypeId, "Input2", L"Ввод2", scada::id::String);
    AddProperty(configuration, id::DataItemType, kObjectOutputPropTypeId, "Output", L"Вывод", scada::id::String);
    AddProperty(configuration, id::DataItemType, kObjectOutputConditionPropTypeId, "OutputCondition", L"Условие (Управление)", scada::id::String);
    AddProperty(configuration, id::DataItemType, kObjectStalePeriodTypeId, "StalePeriod", L"Устаревание (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Simulated, "Simulated", L"Эмуляция", scada::id::Boolean, false);
    CreateReferenceType(configuration, id::DataItemType, id::HasSimulationSignal, "HasSimulationSignal", L"Эмулируемый сигнал", id::SimulationSignalType);
    AddProperty(configuration, id::DataItemType, id::DataItemType_Locked, "Locked", L"Блокирован", scada::id::Boolean, false);
    CreateReferenceType(configuration, id::DataItemType, kObjectHistoricalDbPropTypeId, "ObjectHistoricalDb", L"База данных", id::HistoricalDatabaseType);
  }

  // DiscreteItem
  {
    CreateVariableType(configuration, id::DiscreteItemType, "DiscreteItemType", L"Объект ТС", scada::id::BaseDataType, id::DataItemType);
    AddProperty(configuration, id::DiscreteItemType, kTsInvertedPropTypeId, "Inverted", L"Инверсия", scada::id::Boolean, false);
    CreateReferenceType(configuration, id::DiscreteItemType, id::HasTsFormat, "HasTsFormat", L"Параметры", id::TsFormatType);
  }

  // AnalogItem
  {
    CreateVariableType(configuration, id::AnalogItemType, "AnalogItemType", L"Объект ТИТ", scada::id::BaseDataType, id::DataItemType);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_DisplayFormat, "DisplayFormat", L"Формат", scada::id::String, "0.####");
    AddProperty(configuration, id::AnalogItemType, kTitConversionPropTypeId, "ConversionType", L"Преобразование", scada::id::Int32, 0);
    AddProperty(configuration, id::AnalogItemType, kTitClampingPropTypeId, "ClampingType", L"Ограничение диапазона", scada::id::Boolean, false);
    AddProperty(configuration, id::AnalogItemType, kTitEuLoPropTypeId, "EuLo", L"Лог мин", scada::id::Double, -100);
    AddProperty(configuration, id::AnalogItemType, kTitEuHiPropTypeId, "EuHi", L"Лог макс", scada::id::Double, 100);
    AddProperty(configuration, id::AnalogItemType, kTitIrLoPropTypeId, "IrLo", L"Физ мин", scada::id::Double, 0);
    AddProperty(configuration, id::AnalogItemType, kTitIrHiPropTypeId, "IrHi", L"Физ макс", scada::id::Double, 65535);
    AddProperty(configuration, id::AnalogItemType, kTitLimitLoPropTypeId, "LimitLo", L"Уставка нижняя предаварийная", scada::id::Double);
    AddProperty(configuration, id::AnalogItemType, kTitLimitHiPropTypeId, "LimitHi", L"Уставка верхняя предаварийная", scada::id::Double);
    AddProperty(configuration, id::AnalogItemType, kTitLimitLoLoPropTypeId, "LimitLoLo", L"Уставка нижняя аварийная", scada::id::Double);
    AddProperty(configuration, id::AnalogItemType, kTitLimitHiHiPropTypeId, "LimitHiHi", L"Уставка верхняя аварийная", scada::id::Double);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_EngineeringUnits, "EngineeringUnits", L"Единицы измерения", scada::id::LocalizedText);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_Aperture, "Aperture", L"Апертура", scada::id::Double, 0.0);
    AddProperty(configuration, id::AnalogItemType, id::AnalogItemType_Deadband, "Deadband", L"Мертвая зона", scada::id::Double, 0.0);
  }

  // Modbus Port Device
  {
    CreateObjectType(configuration, id::ModbusLinkType, "ModbusLinkType", L"Направление MODBUS", kLinkTypeId);
    auto* mode_type = CreateDataType(configuration, kModbusPortModeTypeId, "ModbusPortModeType", L"Тип направления MODBUS", scada::id::Int32);
    mode_type->enum_strings = { L"Опрос", L"Ретрансляция", L"Прослушка" };
    AddProperty(configuration, id::ModbusLinkType, kModbusPortModePropTypeId, "Mode", L"Режим", kModbusPortModeTypeId, 0);
  }

  // Modbus Device
  {
    CreateObjectType(configuration, id::ModbusDeviceType, "ModbusDeviceType", L"Устройство MODBUS", id::DeviceType);
    AddComponent(configuration, id::ModbusLinkType, kModbusPort_Devices, id::ModbusDeviceType);
    AddProperty(configuration, id::ModbusDeviceType, kModbusDeviceAddressPropTypeId, "Address", L"Адрес", scada::id::Int32, 1);
    AddProperty(configuration, id::ModbusDeviceType, kModbusDeviceRepeatCountPropTypeId, "RepeatCount", L"Попыток повторов передачи", scada::id::Int32, 3);
  }

  // User
  {
    CreateObjectType(configuration, id::UserType, "UserType", L"Пользователь", scada::id::BaseObjectType);
    AddComponent(configuration, kUsersRootTypeId, kUsersRoot_Users, id::UserType);
    AddProperty(configuration, id::UserType, kUserAccessRightsPropTypeId, "AccessRights", L"Права", scada::id::Int32, 0);
  }

  // IEC Link Device 
  {
    auto* type = CreateObjectType(configuration, id::Iec60870LinkType, "Iec60870LinkType", L"Направление МЭК-60870", kLinkTypeId);
    AddProperty(configuration, id::Iec60870LinkType, id::Iec60870LinkType_Protocol, "Protocol", L"Протокол", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkModePropTypeId, "Mode", L"Режим", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkSendQueueSizePropTypeId, "SendQueueSize", L"Очередь передачи (K)", scada::id::Int32, 12);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkReceiveQueueSizePropTypeId, "ReceiveQueueSize", L"Очередь приема (W)", scada::id::Int32, 8);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkT0PropTypeId, "ConnectTimeout", L"Таймаут связи (T0, с)", scada::id::Int32, 30);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkConfirmationTimeoutPropTypeId, "ConfirmationTimeout", L"Таймаут подтверждения (с)", scada::id::Int32, 5);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkTerminationTimeoutProtocolPropTypeId, "TerminationTimeout", L"Таймаут операции (с)", scada::id::Int32, 20);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkDeviceAddressSizePropTypeId, "DeviceAddressSize", L"Размер адреса устройства", scada::id::Int32, 2);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkCotSizePropTypeId, "CotSize", L"Размер причины передачи", scada::id::Int32, 2);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkInfoAddressSizePropTypeId, "InfoAddressSize", L"Размер адреса объекта", scada::id::Int32, 3);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkCollectDataPropTypeId, "CollectData", L"Сбор данных", scada::id::Boolean, true);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkSendRetriesPropTypeId, "SendRetries", L"Попыток повторов передачи", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkCrcProtectionPropTypeId, "CrcProtection", L"Защита CRC", scada::id::Boolean, false);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkSendTimeoutPropTypeId, "SendTimeout", L"Таймаут передачи (T1, с)", scada::id::Int32, 5);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkReceiveTimeoutPropTypeId, "ReceiveTimeout", L"Таймаут приема (T2, с)", scada::id::Int32, 10);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkIdleTimeoutPropTypeId, "IdleTimeout", L"Таймаут простоя (T3, с)", scada::id::Int32, 30);
    AddProperty(configuration, id::Iec60870LinkType, kIec60870LinkAnonymousPropTypeId, "AnonymousMode", L"Анонимный режим", scada::id::Boolean);
  }

  // IEC Device
  {
    CreateObjectType(configuration, id::Iec60870DeviceType, "Iec60870DeviceType", L"Устройство МЭК-60870", id::DeviceType);
    AddComponent(configuration, id::Iec60870LinkType, kIec60870Link_Devices, id::Iec60870DeviceType);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceAddressPropTypeId, "Address", L"Адрес", scada::id::Int32, 1);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceLinkAddressPropTypeId, "LinkAddress", L"Адрес коммутатора", scada::id::Int32, 1);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceInterrogateOnStartPropTypeId, "InterrogateOnStart", L"Полный опрос при запуске", scada::id::Boolean, true);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceInterrogationPeriodPropTypeId, "InterrogationPeriod", L"Период полного опроса (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceSynchronizeClockOnStartPropTypeId, "SynchronizeClockOnStart", L"Синхронизация часов при запуске", scada::id::Boolean, false);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceClockSynchronizationPeriodPropTypeId, "ClockSynchronizationPeriod", L"Период синхронизации часов (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, id::Iec60870DeviceType_UtcTime, "UtcTime", L"Время UTC", scada::id::Boolean, false);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup1PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 1 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup2PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 2 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup3PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 3 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup4PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 4 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup5PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 5 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup6PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 6 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup7PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 7 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup8PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 8 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup9PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 9 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup10PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 10 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup11PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 11 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup12PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 12 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup13PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 13 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup14PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 14 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup15PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 15 (с)", scada::id::Int32, 0);
    AddProperty(configuration, id::Iec60870DeviceType, kIec60870DeviceGroup16PollPeriodPropTypeId, "Group1PollPeriod", L"Период опроса группы 16 (с)", scada::id::Int32, 0);
  }

  // IEC-61850 Logical Node
  {
    CreateObjectType(configuration, id::Iec61850LogicalNodeType, "Iec61850LogicalNodeType", L"Логический узел МЭК-61850", scada::id::BaseObjectType);
  }

  // IEC-61850 Data Variable
  {
    CreateVariableType(configuration, id::Iec61850DataVariableType, "Iec61850DataVariableType", L"Объект данных МЭК-61850", scada::id::BaseDataType, scada::id::BaseVariableType);
  }

  // IEC-61850 Control Object
  {
    CreateObjectType(configuration, id::Iec61850ControlObjectType, "Iec61850ControlObjectType", L"Объект управления МЭК-61850", scada::id::BaseObjectType);
  }

  // IEC-61850 Device
  {
    CreateObjectType(configuration, id::Iec61850DeviceType, "Iec61850DeviceType", L"Устройство МЭК-61850", id::DeviceType);
    AddComponent(configuration, kDevicesRootTypeId, id::DevicesRoot_Iec61850Devices, id::Iec61850DeviceType);
    {
      auto model = std::make_unique<scada::GenericObject>();
      model->set_id(id::Iec61850DeviceType_Model);
      model->SetBrowseName("Model");
      model->SetDisplayName(L"Модель");
      configuration.AddStaticNode(std::move(model));
      AddReference(configuration, scada::id::HasTypeDefinition, id::Iec61850DeviceType_Model, id::Iec61850LogicalNodeType);
      AddReference(configuration, scada::id::HasModellingRule, id::Iec61850DeviceType_Model, scada::id::ModellingRule_Mandatory);
      // Only HasComponent can be nested now.
      AddReference(configuration, scada::id::HasComponent, id::Iec61850DeviceType, id::Iec61850DeviceType_Model);
    }
    AddProperty(configuration, id::Iec61850DeviceType, id::Iec61850DeviceType_Host, "Host", L"Хост", scada::id::String);
    AddProperty(configuration, id::Iec61850DeviceType, id::Iec61850DeviceType_Port, "Port", L"Порт", scada::id::Int32, 102);
  }

  // IEC-61850 ConfigurableObject
  {
    CreateObjectType(configuration, id::Iec61850ConfigurableObjectType, "ConfigurableObject", L"Параметр МЭК-61850", scada::id::BaseObjectType);
    AddComponent(configuration, id::Iec61850ConfigurableObjectType, id::Iec61850ConfigurableObjectType_Node, id::Iec61850ConfigurableObjectType);
    AddProperty(configuration, id::Iec61850ConfigurableObjectType, id::Iec61850ConfigurableObjectType_Reference, "Address", L"Адрес", scada::id::String);
  }

  // IEC-61850 RCB
  {
    CreateObjectType(configuration, id::Iec61850RcbType, "RCB", L"RCB МЭК-61850", id::Iec61850ConfigurableObjectType);
    AddComponent(configuration, id::Iec61850DeviceType, id::Iec61850DeviceType_Rcb, id::Iec61850RcbType);
  }

  // Server Parameter
  {
    CreateObjectType(configuration, kServerParamTypeId, "ServerParamType", L"Параметр", scada::id::BaseObjectType);
    AddProperty(configuration, kServerParamTypeId, kServerParamValuePropTypeId, "Value", L"Значение", scada::id::String);
  }

  // IEC Retransmission Item
  {
    CreateObjectType(configuration, id::TransmissionItemType, "TransmissionItemType", L"Ретрансляция", scada::id::BaseObjectType);
    AddComponent(configuration, kIecTransmitRootTypeId, kIecTransmitRoot_Items, id::TransmissionItemType);
    CreateReferenceType(configuration, id::TransmissionItemType, kIecTransmitSourceRefTypeId, "IecTransmitSource", L"Объект источник", scada::id::BaseVariableType);
    CreateReferenceType(configuration, id::TransmissionItemType, kIecTransmitTargetDeviceRefTypeId, "IecTransmitTargetDevice", L"Устройство приемник", id::DeviceType);
    AddProperty(configuration, id::TransmissionItemType, kIecTransmitTargetInfoAddressPropTypeId, "InfoAddress", L"Адрес объекта приемника", scada::id::Int32, 1);
  }
}
