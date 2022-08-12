#include "address_space/scada_address_space.h"

#include "address_space/address_space_impl.h"
#include "address_space/node_builder_impl.h"
#include "address_space/node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/strings/utf_string_conversions.h"
#include "common/node_state.h"
#include "core/privileges.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "model/history_node_ids.h"
#include "model/opc_node_ids.h"
#include "model/scada_node_ids.h"
#include "model/security_node_ids.h"

scada::Node* ScadaAddressSpaceBuilder::CreateNode(
    const scada::NodeState& node_state) {
  auto [status, node] = node_factory_.CreateNode(node_state);
  assert(status);
  assert(node);
  return node;
}

scada::ObjectType* ScadaAddressSpaceBuilder::CreateObjectType(
    const scada::NodeId& id,
    scada::QualifiedName qualified_name,
    scada::LocalizedText display_name,
    const scada::NodeId& supertype_id) {
  assert(!address_space_.GetNode(id));

  auto type = std::make_unique<scada::ObjectType>(id, std::move(qualified_name),
                                                  std::move(display_name));
  auto* result = type.get();
  address_space_.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = address_space_.GetMutableNode(supertype_id);
    assert(supertype &&
           supertype->GetNodeClass() == scada::NodeClass::ObjectType);
    AddReference(address_space_, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::VariableType* ScadaAddressSpaceBuilder::CreateVariableType(
    const scada::NodeId& id,
    scada::QualifiedName browse_name,
    scada::LocalizedText display_name,
    const scada::NodeId& data_type_id,
    const scada::NodeId& supertype_id) {
  assert(!address_space_.GetNode(id));

  auto* data_type = address_space_.GetNode(data_type_id);
  assert(data_type && data_type->GetNodeClass() == scada::NodeClass::DataType);

  auto type = std::make_unique<scada::VariableType>(
      id, std::move(browse_name), std::move(display_name),
      *static_cast<const scada::DataType*>(data_type));
  auto* result = type.get();
  address_space_.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = address_space_.GetMutableNode(supertype_id);
    assert(supertype &&
           supertype->GetNodeClass() == scada::NodeClass::VariableType);
    AddReference(address_space_, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::DataType* ScadaAddressSpaceBuilder::CreateDataType(
    const scada::NodeId& id,
    scada::QualifiedName browse_name,
    scada::LocalizedText display_name,
    const scada::NodeId& supertype_id) {
  assert(!address_space_.GetNode(id));

  auto type = std::make_unique<scada::DataType>(id, std::move(browse_name),
                                                std::move(display_name));
  auto* result = type.get();
  address_space_.AddStaticNode(std::move(type));

  if (!supertype_id.is_null()) {
    auto* supertype = address_space_.GetMutableNode(supertype_id);
    assert(supertype &&
           supertype->GetNodeClass() == scada::NodeClass::DataType);
    AddReference(address_space_, scada::id::HasSubtype, *supertype, *result);
  }

  return result;
}

scada::Variable* ScadaAddressSpaceBuilder::AddProperty(
    const scada::NodeId& type_id,
    const scada::NodeId& prop_type_id,
    const scada::NodeId& category_id,
    scada::QualifiedName browse_name,
    scada::LocalizedText display_name,
    const scada::NodeId& data_type_id,
    scada::Variant default_value) {
  assert(!address_space_.GetNode(prop_type_id));

  auto* type = address_space_.GetMutableNode(type_id);
  assert(type && (type->GetNodeClass() == scada::NodeClass::ObjectType ||
                  type->GetNodeClass() == scada::NodeClass::VariableType));

  auto* data_type = AsDataType(address_space_.GetNode(data_type_id));
  assert(data_type);

  auto variable = std::make_unique<scada::GenericVariable>(
      prop_type_id, std::move(browse_name), std::move(display_name),
      *data_type);
  variable->SetValue({default_value, {}, {}, {}});
  auto& result = address_space_.AddStaticNode(std::move(variable));

  AddReference(address_space_, scada::id::HasTypeDefinition, prop_type_id,
               scada::id::PropertyType);
  AddReference(address_space_, scada::id::HasModellingRule, prop_type_id,
               scada::id::ModellingRule_Mandatory);

  AddReference(address_space_, scada::id::HasProperty, *type, result);

  if (!category_id.is_null()) {
    AddReference(address_space_, scada::id::HasPropertyCategory, prop_type_id,
                 category_id);
  }

  return &result;
}

scada::ReferenceType* ScadaAddressSpaceBuilder::CreateReferenceType(
    const scada::NodeId& reference_type_id,
    scada::QualifiedName browse_name,
    scada::LocalizedText display_name,
    const scada::NodeId& supertype_id) {
  assert(!address_space_.GetNode(reference_type_id));

  auto ref_type = std::make_unique<scada::ReferenceType>(
      reference_type_id, std::move(browse_name), std::move(display_name));
  auto& result = address_space_.AddStaticNode(std::move(ref_type));
  AddReference(address_space_, scada::id::HasSubtype, supertype_id,
               reference_type_id);

  return &result;
}

scada::ReferenceType* ScadaAddressSpaceBuilder::CreateReferenceType(
    const scada::NodeId& source_type_id,
    const scada::NodeId& reference_type_id,
    const scada::NodeId& category_id,
    scada::QualifiedName browse_name,
    scada::LocalizedText display_name,
    const scada::NodeId& target_type_id) {
  assert(!address_space_.GetNode(reference_type_id));

  auto* source_type = address_space_.GetMutableNode(source_type_id);
  assert(source_type &&
         (source_type->GetNodeClass() == scada::NodeClass::ObjectType ||
          source_type->GetNodeClass() == scada::NodeClass::VariableType));

  auto* target_type = address_space_.GetMutableNode(target_type_id);
  assert(target_type);

  auto ref_type = std::make_unique<scada::ReferenceType>(
      reference_type_id, std::move(browse_name), std::move(display_name));
  auto& result = address_space_.AddStaticNode(std::move(ref_type));
  AddReference(address_space_, scada::id::HasSubtype,
               scada::id::NonHierarchicalReferences, reference_type_id);

  if (!category_id.is_null()) {
    AddReference(address_space_, scada::id::HasPropertyCategory,
                 reference_type_id, category_id);
  }

  address_space_.AddReference(result, *source_type, *target_type);
  return &result;
}

void ScadaAddressSpaceBuilder::CreateEnumDataType(
    const scada::NodeId& datatype_id,
    const scada::NodeId& enumstrings_id,
    scada::QualifiedName browse_name,
    scada::LocalizedText display_name,
    std::vector<scada::LocalizedText> enum_strings) {
  NodeBuilderImpl builder{address_space_};
  auto* protocol_data_type =
      CreateDataType(datatype_id, std::move(browse_name),
                     std::move(display_name), scada::id::Int32);
  protocol_data_type->enum_strings.emplace(
      builder, *protocol_data_type, enumstrings_id, "EnumStrings",
      scada::LocalizedText{}, scada::id::LocalizedText);
  protocol_data_type->enum_strings->set_value(std::move(enum_strings));
  address_space_.AddNode(*protocol_data_type->enum_strings);
}

scada::ObjectType* ScadaAddressSpaceBuilder::CreateEventType(
    const scada::NodeId& event_type_id,
    const scada::QualifiedName& browse_name) {
  return CreateObjectType(event_type_id, browse_name,
                          scada::ToLocalizedText(browse_name.name()),
                          scada::id::BaseObjectType);
}

void ScadaAddressSpaceBuilder::AddDataVariable(
    const scada::NodeId& type_id,
    const scada::NodeId& variable_decl_id,
    scada::QualifiedName browse_name,
    scada::LocalizedText display_name,
    const scada::NodeId& variable_type_id,
    const scada::NodeId& data_type_id,
    scada::Variant default_value) {
  auto* type = address_space_.GetMutableNode(type_id);
  assert(type);
  assert(type->GetNodeClass() == scada::NodeClass::ObjectType ||
         type->GetNodeClass() == scada::NodeClass::VariableType);

  auto* existing_prop = address_space_.GetMutableNode(variable_decl_id);
  assert(!existing_prop ||
         existing_prop->GetNodeClass() == scada::NodeClass::Variable);

  auto* result = static_cast<scada::Variable*>(existing_prop);
  if (!result) {
    auto* variable_type =
        scada::AsVariableType(address_space_.GetMutableNode(variable_type_id));
    assert(variable_type);
    auto* data_type = scada::AsDataType(address_space_.GetNode(data_type_id));
    assert(data_type);
    auto variable = std::make_unique<scada::GenericVariable>(
        variable_decl_id, std::move(browse_name), std::move(display_name),
        *data_type);
    auto now = base::Time::Now();
    variable->SetValue({std::move(default_value), {}, now, now});
    result = variable.get();
    address_space_.AddStaticNode(std::move(variable));
    AddReference(address_space_, scada::id::HasTypeDefinition, *result,
                 *variable_type);
    AddReference(address_space_, scada::id::HasModellingRule, variable_decl_id,
                 scada::id::ModellingRule_Mandatory);
  }

  AddReference(address_space_, scada::id::HasComponent, *type, *result);
}

void ScadaAddressSpaceBuilder::CreateFileSystemAddressSpace() {
  CreateNode({filesystem::id::FileSystem, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("FileSystem")
                  .set_display_name(u"Файлы")});

  CreateNode({filesystem::id::FileDirectoryType,
              scada::NodeClass::ObjectType,
              {},
              scada::id::BaseObjectType,
              scada::id::HasSubtype,
              scada::NodeAttributes{}
                  .set_browse_name("FileDirectoryType")
                  .set_display_name(u"Папка")});

  AddReference(address_space_, scada::id::Creates, filesystem::id::FileSystem,
               filesystem::id::FileDirectoryType);
  AddReference(address_space_, scada::id::Creates,
               filesystem::id::FileDirectoryType,
               filesystem::id::FileDirectoryType);

  CreateNode({filesystem::id::FileType,
              scada::NodeClass::VariableType,
              {},
              scada::id::BaseVariableType,
              scada::id::HasSubtype,
              scada::NodeAttributes{}
                  .set_browse_name("FileType")
                  .set_display_name(u"Файл")
                  .set_data_type(scada::id::ByteString)});

  CreateNode({filesystem::id::FileType_LastUpdateTime,
              scada::NodeClass::Variable, scada::id::PropertyType,
              filesystem::id::FileType, scada::id::HasProperty,
              scada::NodeAttributes{}
                  .set_browse_name("LastUpdateTime")
                  .set_display_name(u"Время изменения")
                  .set_data_type(scada::id::DateTime)
                  .set_value(scada::DateTime{})});

  CreateNode({filesystem::id::FileType_Size, scada::NodeClass::Variable,
              scada::id::PropertyType, filesystem::id::FileType,
              scada::id::HasProperty,
              scada::NodeAttributes{}
                  .set_browse_name("Size")
                  .set_display_name(u"Размер, байт")
                  .set_data_type(scada::id::UInt64)
                  .set_value(static_cast<scada::UInt64>(0))});

  AddReference(address_space_, scada::id::Creates, filesystem::id::FileSystem,
               filesystem::id::FileType);
  AddReference(address_space_, scada::id::Creates,
               filesystem::id::FileDirectoryType, filesystem::id::FileType);
}

void ScadaAddressSpaceBuilder::CreateSecurityAddressSpace() {
  CreateNode({security::id::Users, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}.set_browse_name("Users").set_display_name(
                  u"Пользователи")});

  // User
  {
    CreateObjectType(security::id::UserType, "UserType", u"Пользователь",
                     scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates, security::id::Users,
                 security::id::UserType);
    AddProperty(security::id::UserType, security::id::UserType_AccessRights, {},
                "AccessRights", u"Права", scada::id::Int32, 0);
  }
}

void ScadaAddressSpaceBuilder::CreateHistoryAddressSpace() {
  CreateNode({history::id::HistoricalDatabases, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("HistoricalDatabases")
                  .set_display_name(u"Базы данных")});

  // Historical DB
  {
    CreateObjectType(history::id::HistoricalDatabaseType,
                     "HistoricalDatabaseType", u"База данных",
                     scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates,
                 history::id::HistoricalDatabases,
                 history::id::HistoricalDatabaseType);
    AddProperty(history::id::HistoricalDatabaseType,
                history::id::HistoricalDatabaseType_Depth, {}, "Depth",
                u"Глубина, дн", scada::id::Int32, 1);
    AddDataVariable(history::id::HistoricalDatabaseType,
                    history::id::HistoricalDatabaseType_WriteValueDuration,
                    "WriteValueDuration", u"Задержка записи измерений, мкс",
                    scada::id::BaseVariableType, scada::id::UInt32);
    AddDataVariable(history::id::HistoricalDatabaseType,
                    history::id::HistoricalDatabaseType_PendingTaskCount,
                    "PendingTaskCount", u"Очередь записи измерений",
                    scada::id::BaseVariableType, scada::id::UInt32);
    AddDataVariable(history::id::HistoricalDatabaseType,
                    history::id::HistoricalDatabaseType_EventCleanupDuration,
                    "EventCleanupDuration",
                    u"Продолжительность очистки событий, мкс",
                    scada::id::BaseVariableType, scada::id::Int32);
    AddDataVariable(history::id::HistoricalDatabaseType,
                    history::id::HistoricalDatabaseType_ValueCleanupDuration,
                    "ValueCleanupDuration",
                    u"Продолжительность очистки измерений, мкс",
                    scada::id::BaseVariableType, scada::id::UInt32);
  }
}

void ScadaAddressSpaceBuilder::CreateDataItemAddressSpace() {
  CreateNode({data_items::id::DataItems, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("DataItems")
                  .set_display_name(u"Все объекты")});

  CreateNode({data_items::id::TsFormats, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("TsFormats")
                  .set_display_name(u"Форматы")});

  CreateNode({data_items::id::SimulationSignals, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("SimulationSignals")
                  .set_display_name(u"Эмулируемые сигналы")});

  // Simulation Item
  {
    CreateVariableType(data_items::id::SimulationSignalType,
                       "SimulationSignalType", u"Эмулируемый сигнал",
                       scada::id::BaseDataType, scada::id::BaseVariableType);
    AddReference(address_space_, scada::id::Creates,
                 data_items::id::SimulationSignals,
                 data_items::id::SimulationSignalType);
    CreateEnumDataType(data_items::id::SimulationFunctionDataType,
                       data_items::id::SimulationFunctionDataType_EnumStrings,
                       "SimulationSignalTypeEnum", u"Тип эмуляции",
                       {u"Случайный", u"Пилообразный", u"Скачкообразный",
                        u"Синус", u"Косинус"});
    AddProperty(data_items::id::SimulationSignalType,
                data_items::id::SimulationSignalType_Function, {}, "Type",
                u"Тип", data_items::id::SimulationFunctionDataType,
                static_cast<scada::Int32>(0));
    AddProperty(data_items::id::SimulationSignalType,
                data_items::id::SimulationSignalType_Period, {}, "Period",
                u"Период, мс", scada::id::Int32, 60000);
    AddProperty(data_items::id::SimulationSignalType,
                data_items::id::SimulationSignalType_Phase, {}, "Phase",
                u"Фаза, мс", scada::id::Int32, 0);
    AddProperty(data_items::id::SimulationSignalType,
                data_items::id::SimulationSignalType_UpdateInterval, {},
                "UpdateInterval", u"Обновление, мс", scada::id::Int32, 1000);
  }

  // TsFormat
  {
    CreateObjectType(data_items::id::TsFormatType, "TsFormatType", u"Формат",
                     scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates, data_items::id::TsFormats,
                 data_items::id::TsFormatType);
    AddProperty(data_items::id::TsFormatType,
                data_items::id::TsFormatType_OpenLabel, {}, "OpenLabel",
                u"Подпись 0", scada::id::LocalizedText, scada::LocalizedText{});
    AddProperty(data_items::id::TsFormatType,
                data_items::id::TsFormatType_CloseLabel, {}, "CloseLabel",
                u"Подпись 1", scada::id::LocalizedText, scada::LocalizedText{});
    AddProperty(data_items::id::TsFormatType,
                data_items::id::TsFormatType_OpenColor, {}, "OpenColor",
                u"Цвет 0", scada::id::Int32, 0);
    AddProperty(data_items::id::TsFormatType,
                data_items::id::TsFormatType_CloseColor, {}, "CloseColor",
                u"Цвет 1", scada::id::Int32, 0);
  }

  // DataGroup
  {
    CreateObjectType(data_items::id::DataGroupType, "DataGroupType", u"Группа",
                     scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates, data_items::id::DataItems,
                 data_items::id::DataGroupType);
    AddReference(address_space_, scada::id::Creates,
                 data_items::id::DataGroupType,
                 data_items::id::DataGroupType);  // recursive
    AddProperty(data_items::id::DataGroupType,
                data_items::id::DataGroupType_Simulated,
                data_items::id::SimulationPropertyCategory, "Simulated",
                u"Эмуляция", scada::id::Boolean, false);
  }

  // DataItem
  {
    CreateVariableType(data_items::id::DataItemType, "DataItemType", u"Объект",
                       scada::id::BaseDataType, scada::id::BaseVariableType);
    AddReference(address_space_, scada::id::Creates,
                 data_items::id::DataGroupType, data_items::id::DataItemType);
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Alias, {}, "Alias", u"Алиас",
                scada::id::String, scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Severity, {}, "Severity",
                u"Важность", scada::id::Int32, 1);
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Input1,
                data_items::id::ChannelsPropertyCategory, "Input1",
                u"Основной ввод", scada::id::String, scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Input2,
                data_items::id::ChannelsPropertyCategory, "Input2",
                u"Резервный ввод", scada::id::String, scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Locked,
                data_items::id::ChannelsPropertyCategory, "Locked",
                u"Блокировка", scada::id::Boolean, false);
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_StalePeriod,
                data_items::id::ChannelsPropertyCategory, "StalePeriod",
                u"Устаревание, с", scada::id::Int32, 0);
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Output,
                data_items::id::ChannelsPropertyCategory, "Output", u"Вывод",
                scada::id::String, scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_OutputTwoStaged,
                data_items::id::ChannelsPropertyCategory, "OutputTwoStaged",
                u"Двухэтапное управление", scada::id::Boolean, true);
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_OutputCondition,
                data_items::id::ChannelsPropertyCategory, "OutputCondition",
                u"Условие управления", scada::id::String, scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Simulated,
                data_items::id::SimulationPropertyCategory, "Simulated",
                u"Эмуляция", scada::id::Boolean, false);
    CreateReferenceType(
        data_items::id::DataItemType, data_items::id::HasSimulationSignal,
        data_items::id::SimulationPropertyCategory, "HasSimulationSignal",
        u"Эмулируемый сигнал", data_items::id::SimulationSignalType);
  }

  // DiscreteItem
  {
    CreateVariableType(data_items::id::DiscreteItemType, "DiscreteItemType",
                       u"Объект ТС", scada::id::BaseDataType,
                       data_items::id::DataItemType);
    AddProperty(data_items::id::DiscreteItemType,
                data_items::id::DiscreteItemType_Inversion,
                data_items::id::ConversionPropertyCategory, "Inverted",
                u"Инверсия", scada::id::Boolean, false);
    CreateReferenceType(data_items::id::DiscreteItemType,
                        data_items::id::HasTsFormat,
                        data_items::id::DisplayPropertyCategory, "HasTsFormat",
                        u"Параметры", data_items::id::TsFormatType);
  }

  // AnalogItem
  {
    CreateVariableType(data_items::id::AnalogItemType, "AnalogItemType",
                       u"Объект ТИТ", scada::id::BaseDataType,
                       data_items::id::DataItemType);
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_DisplayFormat,
                data_items::id::DisplayPropertyCategory, "DisplayFormat",
                u"Формат", scada::id::String, "0.####");
    CreateEnumDataType(data_items::id::AnalogConversionDataType,
                       data_items::id::AnalogConversionDataType_EnumStrings,
                       "AnalogConversionDataType", u"Преобразование",
                       {u"Нет", u"Линейное"});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_Conversion,
                data_items::id::ConversionPropertyCategory, "Conversion",
                u"Преобразование", data_items::id::AnalogConversionDataType,
                static_cast<scada::Int32>(0));
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_Clamping,
                data_items::id::FilteringPropertyCategory, "Clamping",
                u"Ограничение диапазона", scada::id::Boolean, false);
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_EuLo,
                data_items::id::ConversionPropertyCategory, "EuLo",
                u"Логический минимимум", scada::id::Double, -100.0);
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_EuHi,
                data_items::id::ConversionPropertyCategory, "EuHi",
                u"Логический максимум", scada::id::Double, 100.0);
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_IrLo,
                data_items::id::ConversionPropertyCategory, "IrLo",
                u"Физический минимимум", scada::id::Double, 0.0);
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_IrHi,
                data_items::id::ConversionPropertyCategory, "IrHi",
                u"Физический максимум", scada::id::Double, 65535.0);
    // TODO: Variant.
    AddProperty(
        data_items::id::AnalogItemType, data_items::id::AnalogItemType_LimitLo,
        data_items::id::LimitsPropertyCategory, "LimitLo",
        u"Уставка нижняя предаварийная", scada::id::Double, scada::Variant{});
    AddProperty(
        data_items::id::AnalogItemType, data_items::id::AnalogItemType_LimitHi,
        data_items::id::LimitsPropertyCategory, "LimitHi",
        u"Уставка верхняя предаварийная", scada::id::Double, scada::Variant{});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_LimitLoLo,
                data_items::id::LimitsPropertyCategory, "LimitLoLo",
                u"Уставка нижняя аварийная", scada::id::Double,
                scada::Variant{});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_LimitHiHi,
                data_items::id::LimitsPropertyCategory, "LimitHiHi",
                u"Уставка верхняя аварийная", scada::id::Double,
                scada::Variant{});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_EngineeringUnits,
                data_items::id::DisplayPropertyCategory, "EngineeringUnits",
                u"Единицы измерения", scada::id::LocalizedText,
                scada::LocalizedText{});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_Aperture,
                data_items::id::FilteringPropertyCategory, "Aperture",
                u"Апертура", scada::id::Double, 0.0);
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_Deadband,
                data_items::id::FilteringPropertyCategory, "Deadband",
                u"Мертвая зона", scada::id::Double, 0.0);
  }

  // Aliases
  {
    CreateNode(
        {data_items::id::Aliases, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::ObjectsFolder, scada::id::Organizes,
         scada::NodeAttributes{}.set_browse_name("Aliases").set_display_name(
             u"Алиасы")});

    CreateObjectType(data_items::id::AliasType, "AliasType", u"Алиас",
                     scada::id::BaseObjectType);
    CreateReferenceType(data_items::id::AliasOf, "AliasOf", {},
                        scada::id::NonHierarchicalReferences);
  }
}

void ScadaAddressSpaceBuilder::CreateDeviceAddressSpace() {
  CreateNode(
      {devices::id::Devices, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}.set_browse_name("Devices").set_display_name(
           u"Все оборудование")});

  CreateNode({devices::id::TransmissionItems, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("TransmissionItems")
                  .set_display_name(u"Ретрансляция")});

  // Device
  {
    CreateObjectType(devices::id::DeviceType, "DeviceType", u"Устройство",
                     scada::id::BaseObjectType);
    AddProperty(devices::id::DeviceType, devices::id::DeviceType_Disabled, {},
                "Disabled", u"Отключено", scada::id::Boolean, true);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_Online,
                    "Online", u"Связь", scada::id::BaseVariableType,
                    scada::id::Boolean, false);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_Enabled,
                    "Enabled", u"Включено", scada::id::BaseVariableType,
                    scada::id::Boolean, false);
    AddDataVariable(devices::id::DeviceType,
                    devices::id::DeviceType_MessagesOut, "MessagesOut",
                    u"Отправлено сообщений", scada::id::BaseVariableType,
                    scada::id::Int32, 0);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_MessagesIn,
                    "MessagesIn", u"Принято сообщений",
                    scada::id::BaseVariableType, scada::id::Int32, 0);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_BytesOut,
                    "BytesOut", u"Отправлено байт", scada::id::BaseVariableType,
                    scada::id::Int32, 0);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_BytesIn,
                    "BytesIn", u"Принято байт", scada::id::BaseVariableType,
                    scada::id::Int32, 0);
    CreateEventType(devices::id::DeviceWatchEventType, "DeviceWatchEventType");
  }

  // Link
  {
    CreateObjectType(devices::id::LinkType, "LinkType", u"Направление",
                     devices::id::DeviceType);
    AddReference(address_space_, scada::id::Creates, devices::id::Devices,
                 devices::id::LinkType);
    AddProperty(devices::id::LinkType, devices::id::LinkType_Transport, {},
                "TransportString", u"Транспорт", scada::id::String,
                scada::String{});
    AddDataVariable(devices::id::LinkType, devices::id::LinkType_ConnectCount,
                    "ConnectCount", u"ConnectCount",
                    scada::id::BaseVariableType, scada::id::Int32, 0);
    AddDataVariable(devices::id::LinkType,
                    devices::id::LinkType_ActiveConnections,
                    "ActiveConnections", u"ActiveConnections",
                    scada::id::BaseVariableType, scada::id::Int32, 0);
  }

  // MODBUS Protocol

  CreateEnumDataType(devices::id::ModbusLinkProtocol,
                     devices::id::ModbusLinkProtocol_EnumStrings,
                     "ModbusLinkProtocol", u"Тип протокола MODBUS",
                     {u"RTU", u"ASCII", u"TCP"});

  // MODBUS Mode

  CreateEnumDataType(devices::id::ModbusModeDataType,
                     devices::id::ModbusModeDataType_EnumStrings,
                     "ModbusModeDataType", u"Режим MODBUS",
                     {u"Опрос", u"Ретрансляция"});

  // Modbus Port Device
  {
    CreateObjectType(devices::id::ModbusLinkType, "ModbusLinkType",
                     u"Направление MODBUS", devices::id::LinkType);
    AddProperty(devices::id::ModbusLinkType,
                devices::id::ModbusLinkType_Protocol, {}, "Protocol",
                u"Протокол", devices::id::ModbusLinkProtocol, 0);
    AddProperty(devices::id::ModbusLinkType, devices::id::ModbusLinkType_Mode,
                {}, "Mode", u"Режим", devices::id::ModbusModeDataType, 0);
    AddProperty(devices::id::ModbusLinkType,
                devices::id::ModbusLinkType_RequestDelay, {}, "RequestDelay",
                u"Задержка запроса, мс", scada::id::Int32, 0);
  }

  // Modbus Device
  {
    CreateObjectType(devices::id::ModbusDeviceType, "ModbusDeviceType",
                     u"Устройство MODBUS", devices::id::DeviceType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::ModbusLinkType, devices::id::ModbusDeviceType);
    AddProperty(devices::id::ModbusDeviceType,
                devices::id::ModbusDeviceType_Address, {}, "Address", u"Адрес",
                scada::id::UInt8, 1);
    AddProperty(devices::id::ModbusDeviceType,
                devices::id::ModbusDeviceType_SendRetryCount, {}, "RepeatCount",
                u"Попыток повторов передачи", scada::id::Int32, 3);
    AddProperty(devices::id::ModbusDeviceType,
                devices::id::ModbusDeviceType_ResponseTimeout, {},
                "ResponseTimeout", u"Таймаут ответа, мс", scada::id::Int32,
                1000);
  }

  // IEC-60870 Protocol

  CreateEnumDataType(devices::id::Iec60870ProtocolDataType,
                     devices::id::Iec60870ProtocolDataType_EnumStrings,
                     "Iec60870ProtocolType", u"Тип протокола МЭК-60870",
                     {u"МЭК-60870-104", u"МЭК-60870-101"});

  // IEC-60870 Mode

  CreateEnumDataType(devices::id::Iec60870ModeDataType,
                     devices::id::Iec60870ModeDataType_EnumStrings,
                     "Iec60870ModeType", u"Режим МЭК-60870",
                     {u"Опрос", u"Ретрансляция", u"Прослушка"});

  // IEC-60870 Link Device
  {
    CreateObjectType(devices::id::Iec60870LinkType, "Iec60870LinkType",
                     u"Направление МЭК-60870", devices::id::LinkType);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_Protocol, {}, "Protocol",
                u"Протокол", devices::id::Iec60870ProtocolDataType,
                static_cast<scada::Int32>(0));
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_Mode, {}, "Mode", u"Режим",
                devices::id::Iec60870ModeDataType,
                static_cast<scada::Int32>(0));
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_SendQueueSize, {},
                "SendQueueSize", u"Очередь передачи (K)", scada::id::Int32, 12);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_ReceiveQueueSize, {},
                "ReceiveQueueSize", u"Очередь приема (W)", scada::id::Int32, 8);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_ConnectTimeout, {},
                "ConnectTimeout", u"Таймаут связи (T0), с", scada::id::Int32,
                30);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_ConfirmationTimeout, {},
                "ConfirmationTimeout", u"Таймаут подтверждения, с",
                scada::id::Int32, 5);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_TerminationTimeout, {},
                "TerminationTimeout", u"Таймаут операции, с", scada::id::Int32,
                20);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_DeviceAddressSize, {},
                "DeviceAddressSize", u"Размер адреса устройства",
                scada::id::UInt8, 2);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_COTSize, {}, "CotSize",
                u"Размер причины передачи", scada::id::UInt8, 2);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_InfoAddressSize, {},
                "InfoAddressSize", u"Размер адреса объекта", scada::id::UInt8,
                3);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_DataCollection, {}, "CollectData",
                u"Сбор данных", scada::id::Boolean, true);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_SendRetryCount, {}, "SendRetries",
                u"Попыток повторов передачи", scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_CRCProtection, {},
                "CrcProtection", u"Защита CRC", scada::id::Boolean, false);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_SendTimeout, {}, "SendTimeout",
                u"Таймаут передачи (T1), с", scada::id::Int32, 15);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_ReceiveTimeout, {},
                "ReceiveTimeout", u"Таймаут приема (T2), с", scada::id::Int32,
                10);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_IdleTimeout, {}, "IdleTimeout",
                u"Таймаут простоя (T3), с", scada::id::Int32, 20);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_AnonymousMode, {},
                "AnonymousMode", u"Анонимный режим", scada::id::Boolean, false);
  }

  // IEC-60870 Device
  {
    CreateObjectType(devices::id::Iec60870DeviceType, "Iec60870DeviceType",
                     u"Устройство МЭК-60870", devices::id::DeviceType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::Iec60870LinkType,
                 devices::id::Iec60870DeviceType);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_Address, {}, "Address",
                u"Адрес", scada::id::UInt16, 1);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_LinkAddress, {}, "LinkAddress",
                u"Адрес коммутатора", scada::id::UInt16, 1);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_StartupInterrogation, {},
                "InterrogateOnStart", u"Полный опрос при запуске",
                scada::id::Boolean, true);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriod, {},
                "InterrogationPeriod", u"Период полного опроса, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_StartupClockSync, {},
                "SynchronizeClockOnStart", u"Синхронизация часов при запуске",
                scada::id::Boolean, false);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_ClockSyncPeriod, {},
                "ClockSynchronizationPeriod", u"Период синхронизации часов, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_UtcTime, {}, "UtcTime",
                u"Время UTC", scada::id::Boolean, false);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup1, {},
                "Group1PollPeriod", u"Период опроса группы 1, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup2, {},
                "Group2PollPeriod", u"Период опроса группы 2, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup3, {},
                "Group3PollPeriod", u"Период опроса группы 3, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup4, {},
                "Group4PollPeriod", u"Период опроса группы 4, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup5, {},
                "Group5PollPeriod", u"Период опроса группы 5, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup6, {},
                "Group6PollPeriod", u"Период опроса группы 6, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup7, {},
                "Group7PollPeriod", u"Период опроса группы 7, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup8, {},
                "Group8PollPeriod", u"Период опроса группы 8, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup9, {},
                "Group9PollPeriod", u"Период опроса группы 9, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup10, {},
                "Group10PollPeriod", u"Период опроса группы 10, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup11, {},
                "Group11PollPeriod", u"Период опроса группы 11, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup12, {},
                "Group12PollPeriod", u"Период опроса группы 12, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup13, {},
                "Group13PollPeriod", u"Период опроса группы 13, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup14, {},
                "Group14PollPeriod", u"Период опроса группы 14, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup15, {},
                "Group15PollPeriod", u"Период опроса группы 15, с",
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup16, {},
                "Group16PollPeriod", u"Период опроса группы 16, с",
                scada::id::Int32, 0);
  }

  // IEC-61850 Logical Node
  {
    CreateObjectType(devices::id::Iec61850LogicalNodeType,
                     "Iec61850LogicalNodeType", u"Логический узел МЭК-61850",
                     scada::id::BaseObjectType);
  }

  // IEC-61850 Data Variable
  {
    CreateVariableType(devices::id::Iec61850DataVariableType,
                       "Iec61850DataVariableType", u"Объект данных МЭК-61850",
                       scada::id::BaseDataType, scada::id::BaseVariableType);
  }

  // IEC-61850 Control Object
  {
    CreateObjectType(devices::id::Iec61850ControlObjectType,
                     "Iec61850ControlObjectType",
                     u"Объект управления МЭК-61850", scada::id::BaseObjectType);
  }

  // IEC-61850 Device
  {
    CreateObjectType(devices::id::Iec61850DeviceType, "Iec61850DeviceType",
                     u"Устройство МЭК-61850", devices::id::DeviceType);
    AddReference(address_space_, scada::id::Creates, devices::id::Devices,
                 devices::id::Iec61850DeviceType);
    {
      auto model = std::make_unique<scada::GenericObject>();
      model->set_id(devices::id::Iec61850DeviceType_Model);
      model->SetBrowseName("Model");
      model->SetDisplayName(u"Модель");
      address_space_.AddStaticNode(std::move(model));
      AddReference(address_space_, scada::id::HasTypeDefinition,
                   devices::id::Iec61850DeviceType_Model,
                   devices::id::Iec61850LogicalNodeType);
      AddReference(address_space_, scada::id::HasModellingRule,
                   devices::id::Iec61850DeviceType_Model,
                   scada::id::ModellingRule_Mandatory);
      // Only HasComponent can be nested now.
      AddReference(address_space_, scada::id::HasComponent,
                   devices::id::Iec61850DeviceType,
                   devices::id::Iec61850DeviceType_Model);
    }
    AddProperty(devices::id::Iec61850DeviceType,
                devices::id::Iec61850DeviceType_Host, {}, "Host", u"Хост",
                scada::id::String, scada::String{});
    AddProperty(devices::id::Iec61850DeviceType,
                devices::id::Iec61850DeviceType_Port, {}, "Port", u"Порт",
                scada::id::Int32, 102);
  }

  // IEC-61850 ConfigurableObject
  {
    CreateObjectType(devices::id::Iec61850ConfigurableObjectType,
                     "ConfigurableObject", u"Параметр МЭК-61850",
                     scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::Iec61850ConfigurableObjectType,
                 devices::id::Iec61850ConfigurableObjectType);
    AddProperty(devices::id::Iec61850ConfigurableObjectType,
                devices::id::Iec61850ConfigurableObjectType_Reference, {},
                "Address", u"Адрес", scada::id::String, scada::String{});
  }

  // IEC-61850 RCB
  {
    CreateObjectType(devices::id::Iec61850RcbType, "RCB", u"RCB МЭК-61850",
                     devices::id::Iec61850ConfigurableObjectType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::Iec61850DeviceType, devices::id::Iec61850RcbType);
  }

  // IEC Retransmission Item
  {
    CreateObjectType(devices::id::TransmissionItemType, "TransmissionItemType",
                     u"Ретрансляция", scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::TransmissionItems,
                 devices::id::TransmissionItemType);
    CreateReferenceType(devices::id::TransmissionItemType,
                        devices::id::HasTransmissionSource, {},
                        "IecTransmitSource", u"Объект источник",
                        scada::id::BaseVariableType);
    CreateReferenceType(devices::id::TransmissionItemType,
                        devices::id::HasTransmissionTarget, {},
                        "IecTransmitTargetDevice", u"Устройство приемник",
                        devices::id::DeviceType);
    AddProperty(devices::id::TransmissionItemType,
                devices::id::TransmissionItemType_SourceAddress, {},
                "InfoAddress", u"Адрес объекта приемника", scada::id::Int32, 1);
  }
}

void ScadaAddressSpaceBuilder::CreateOpcAddressSpace() {
  CreateNode({opc::id::OPC, scada::NodeClass::Object, scada::id::FolderType,
              scada::id::ObjectsFolder, scada::id::Organizes,
              scada::NodeAttributes{}.set_browse_name("OPC").set_display_name(
                  u"OPC")});
}

void ScadaAddressSpaceBuilder::CreateScadaAddressSpace() {
  CreateEventType(scada::id::GeneralModelChangeEventType,
                  "GeneralModelChangeEventType");
  CreateEventType(scada::id::SemanticChangeEventType,
                  "SemanticChangeEventType");
  CreateEventType(scada::id::SystemEventType, "SystemEventType");

  CreateReferenceType(scada::id::Creates, "Creates", u"Можно создать",
                      scada::id::NonHierarchicalReferences);

  // MetricType.
  {
    CreateVariableType(scada::id::MetricType, "MetricType", u"Метрика",
                       scada::id::Int64, scada::id::BaseVariableType);
    AddProperty(scada::id::MetricType, scada::id::MetricType_AggregateFunction,
                {}, "AggregateFunction", u"Функция", scada::id::NodeId, {});
    AddProperty(scada::id::MetricType, scada::id::MetricType_AggregateInterval,
                {}, "AggregateInterval", u"Период, с", scada::id::Int32, {});
  }

  // Property Categories.
  {
    CreateNode({scada::id::HasPropertyCategory,
                scada::NodeClass::ReferenceType,
                {},
                scada::id::NonHierarchicalReferences,
                scada::id::HasSubtype,
                scada::NodeAttributes{}
                    .set_browse_name("HasPropertyCategory")
                    .set_display_name(u"Категория свойства")});

    CreateNode({scada::id::PropertyCategories, scada::NodeClass::Object,
                scada::id::FolderType, scada::id::ObjectsFolder,
                scada::id::Organizes,
                scada::NodeAttributes{}
                    .set_browse_name("PropertyCategories")
                    .set_display_name(u"Категории свойств")});

    CreateNode(
        {scada::id::GeneralPropertyCategory, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::PropertyCategories,
         scada::id::Organizes,
         scada::NodeAttributes().set_browse_name("General").set_display_name(
             u"Общие")});

    CreateNode({data_items::id::ChannelsPropertyCategory,
                scada::NodeClass::Object, scada::id::FolderType,
                scada::id::PropertyCategories, scada::id::Organizes,
                scada::NodeAttributes()
                    .set_browse_name("Channels")
                    .set_display_name(u"Каналы")});

    CreateNode({data_items::id::ConversionPropertyCategory,
                scada::NodeClass::Object, scada::id::FolderType,
                scada::id::PropertyCategories, scada::id::Organizes,
                scada::NodeAttributes()
                    .set_browse_name("Conversion")
                    .set_display_name(u"Преобразование")});

    CreateNode({data_items::id::FilteringPropertyCategory,
                scada::NodeClass::Object, scada::id::FolderType,
                scada::id::PropertyCategories, scada::id::Organizes,
                scada::NodeAttributes()
                    .set_browse_name("Filtering")
                    .set_display_name(u"Фильтрация")});

    CreateNode(
        {data_items::id::DisplayPropertyCategory, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::PropertyCategories,
         scada::id::Organizes,
         scada::NodeAttributes().set_browse_name("Display").set_display_name(
             u"Отображение")});

    CreateNode(
        {history::id::HistoryPropertyCategory, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::PropertyCategories,
         scada::id::Organizes,
         scada::NodeAttributes().set_browse_name("History").set_display_name(
             u"Архивирование")});

    CreateNode({data_items::id::SimulationPropertyCategory,
                scada::NodeClass::Object, scada::id::FolderType,
                scada::id::PropertyCategories, scada::id::Organizes,
                scada::NodeAttributes()
                    .set_browse_name("Simulation")
                    .set_display_name(u"Эмуляция")});

    CreateNode(
        {data_items::id::LimitsPropertyCategory, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::PropertyCategories,
         scada::id::Organizes,
         scada::NodeAttributes().set_browse_name("Limits").set_display_name(
             u"Уставки")});
  }
}

void ScadaAddressSpaceBuilder::BuildAll() {
  CreateScadaAddressSpace();
  CreateSecurityAddressSpace();
  CreateFileSystemAddressSpace();
  CreateHistoryAddressSpace();
  CreateDataItemAddressSpace();
  CreateDeviceAddressSpace();
  CreateOpcAddressSpace();

  CreateReferenceType(data_items::id::DataGroupType, data_items::id::HasDevice,
                      data_items::id::ChannelsPropertyCategory, "HasDevice",
                      u"Устройство", devices::id::DeviceType);

  CreateReferenceType(
      data_items::id::DataItemType, history::id::HasHistoricalDatabase,
      history::id::HistoryPropertyCategory, "HasHistoricalDatabase",
      u"Архив значений", history::id::HistoricalDatabaseType);
  CreateReferenceType(devices::id::DeviceType, history::id::HasEventDatabase,
                      history::id::HistoryPropertyCategory, "HasEventDatabase",
                      u"Архив событий", history::id::HistoricalDatabaseType);
}
