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
#include "core/configuration_types.h"
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

  AddReference(result, *source_type, *target_type);
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

void ScadaAddressSpaceBuilder::AddDataVariable(
    const scada::NodeId& type_id,
    const scada::NodeId& variable_decl_id,
    scada::QualifiedName browse_name,
    scada::LocalizedText display_name,
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
    auto* variable_type = scada::AsVariableType(
        address_space_.GetMutableNode(scada::id::BaseVariableType));
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
                  .set_display_name(base::WideToUTF16(L"Файлы"))});

  CreateNode({filesystem::id::FileDirectoryType,
              scada::NodeClass::ObjectType,
              {},
              scada::id::BaseObjectType,
              scada::id::HasSubtype,
              scada::NodeAttributes{}
                  .set_browse_name("FileDirectoryType")
                  .set_display_name(base::WideToUTF16(L"Папка"))});

  CreateNode({filesystem::id::FileType,
              scada::NodeClass::VariableType,
              {},
              scada::id::BaseVariableType,
              scada::id::HasSubtype,
              scada::NodeAttributes{}
                  .set_browse_name("FileType")
                  .set_display_name(base::WideToUTF16(L"Файл"))
                  .set_data_type(scada::id::ByteString)});

  CreateNode({filesystem::id::FileType_LastUpdateTime,
              scada::NodeClass::Variable, scada::id::PropertyType,
              filesystem::id::FileType, scada::id::HasProperty,
              scada::NodeAttributes{}
                  .set_browse_name("LastUpdateTime")
                  .set_display_name(base::WideToUTF16(L"Время изменения"))
                  .set_data_type(scada::id::DateTime)
                  .set_value(scada::DateTime{})});

  CreateNode({filesystem::id::FileType_Size, scada::NodeClass::Variable,
              scada::id::PropertyType, filesystem::id::FileType,
              scada::id::HasProperty,
              scada::NodeAttributes{}
                  .set_browse_name("Size")
                  .set_display_name(base::WideToUTF16(L"Размер, байт"))
                  .set_data_type(scada::id::UInt64)
                  .set_value(static_cast<scada::UInt64>(0))});
}

void ScadaAddressSpaceBuilder::CreateScadaAddressSpace() {
  CreateReferenceType(scada::id::Creates, "Creates",
                      base::WideToUTF16(L"Можно создать"),
                      scada::id::NonHierarchicalReferences);

  CreateNode({data_items::id::DataItems, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("DataItems")
                  .set_display_name(base::WideToUTF16(L"Все объекты"))});

  CreateNode(
      {devices::id::Devices, scada::NodeClass::Object, scada::id::FolderType,
       scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}.set_browse_name("Devices").set_display_name(
           base::WideToUTF16(L"Все оборудование"))});

  CreateNode({security::id::Users, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}.set_browse_name("Users").set_display_name(
                  base::WideToUTF16(L"Пользователи"))});

  CreateNode({data_items::id::TsFormats, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("TsFormats")
                  .set_display_name(base::WideToUTF16(L"Форматы"))});

  CreateNode(
      {data_items::id::SimulationSignals, scada::NodeClass::Object,
       scada::id::FolderType, scada::id::ObjectsFolder, scada::id::Organizes,
       scada::NodeAttributes{}
           .set_browse_name("SimulationSignals")
           .set_display_name(base::WideToUTF16(L"Эмулируемые сигналы"))});

  CreateNode({history::id::HistoricalDatabases, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("HistoricalDatabases")
                  .set_display_name(base::WideToUTF16(L"Базы данных"))});

  CreateNode({devices::id::TransmissionItems, scada::NodeClass::Object,
              scada::id::FolderType, scada::id::ObjectsFolder,
              scada::id::Organizes,
              scada::NodeAttributes{}
                  .set_browse_name("TransmissionItems")
                  .set_display_name(base::WideToUTF16(L"Ретрансляция"))});

  // Property Categories.
  {
    CreateNode(
        {scada::id::HasPropertyCategory,
         scada::NodeClass::ReferenceType,
         {},
         scada::id::NonHierarchicalReferences,
         scada::id::HasSubtype,
         scada::NodeAttributes{}
             .set_browse_name("HasPropertyCategory")
             .set_display_name(base::WideToUTF16(L"Категория свойства"))});

    CreateNode(
        {scada::id::PropertyCategories, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::ObjectsFolder, scada::id::Organizes,
         scada::NodeAttributes{}
             .set_browse_name("PropertyCategories")
             .set_display_name(base::WideToUTF16(L"Категории свойств"))});

    CreateNode(
        {scada::id::GeneralPropertyCategory, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::PropertyCategories,
         scada::id::Organizes,
         scada::NodeAttributes().set_browse_name("General").set_display_name(
             base::WideToUTF16(L"Общие"))});

    CreateNode({data_items::id::ChannelsPropertyCategory,
                scada::NodeClass::Object, scada::id::FolderType,
                scada::id::PropertyCategories, scada::id::Organizes,
                scada::NodeAttributes()
                    .set_browse_name("Channels")
                    .set_display_name(base::WideToUTF16(L"Каналы"))});

    CreateNode({data_items::id::ConversionPropertyCategory,
                scada::NodeClass::Object, scada::id::FolderType,
                scada::id::PropertyCategories, scada::id::Organizes,
                scada::NodeAttributes()
                    .set_browse_name("Conversion")
                    .set_display_name(base::WideToUTF16(L"Преобразование"))});

    CreateNode({data_items::id::FilteringPropertyCategory,
                scada::NodeClass::Object, scada::id::FolderType,
                scada::id::PropertyCategories, scada::id::Organizes,
                scada::NodeAttributes()
                    .set_browse_name("Filtering")
                    .set_display_name(base::WideToUTF16(L"Фильтрация"))});

    CreateNode(
        {data_items::id::DisplayPropertyCategory, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::PropertyCategories,
         scada::id::Organizes,
         scada::NodeAttributes().set_browse_name("Display").set_display_name(
             base::WideToUTF16(L"Отображение"))});

    CreateNode(
        {history::id::HistoryPropertyCategory, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::PropertyCategories,
         scada::id::Organizes,
         scada::NodeAttributes().set_browse_name("History").set_display_name(
             base::WideToUTF16(L"Архивирование"))});

    CreateNode({data_items::id::SimulationPropertyCategory,
                scada::NodeClass::Object, scada::id::FolderType,
                scada::id::PropertyCategories, scada::id::Organizes,
                scada::NodeAttributes()
                    .set_browse_name("Simulation")
                    .set_display_name(base::WideToUTF16(L"Эмуляция"))});

    CreateNode(
        {data_items::id::LimitsPropertyCategory, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::PropertyCategories,
         scada::id::Organizes,
         scada::NodeAttributes().set_browse_name("Limits").set_display_name(
             base::WideToUTF16(L"Уставки"))});
  }

  // Simulation Item
  {
    CreateVariableType(data_items::id::SimulationSignalType,
                       "SimulationSignalType",
                       base::WideToUTF16(L"Эмулируемый сигнал"),
                       scada::id::BaseDataType, scada::id::BaseVariableType);
    AddReference(address_space_, scada::id::Creates,
                 data_items::id::SimulationSignals,
                 data_items::id::SimulationSignalType);
    CreateEnumDataType(
        data_items::id::SimulationFunctionDataType,
        data_items::id::SimulationFunctionDataType_EnumStrings,
        "SimulationSignalTypeEnum", base::WideToUTF16(L"Тип эмуляции"),
        {base::WideToUTF16(L"Случайный"), base::WideToUTF16(L"Пилообразный"),
         base::WideToUTF16(L"Скачкообразный"), base::WideToUTF16(L"Синус"),
         base::WideToUTF16(L"Косинус")});
    AddProperty(data_items::id::SimulationSignalType,
                data_items::id::SimulationSignalType_Function, {}, "Type",
                base::WideToUTF16(L"Тип"),
                data_items::id::SimulationFunctionDataType,
                static_cast<scada::Int32>(0));
    AddProperty(data_items::id::SimulationSignalType,
                data_items::id::SimulationSignalType_Period, {}, "Period",
                base::WideToUTF16(L"Период, мс"), scada::id::Int32, 60000);
    AddProperty(data_items::id::SimulationSignalType,
                data_items::id::SimulationSignalType_Phase, {}, "Phase",
                base::WideToUTF16(L"Фаза, мс"), scada::id::Int32, 0);
    AddProperty(data_items::id::SimulationSignalType,
                data_items::id::SimulationSignalType_UpdateInterval, {},
                "UpdateInterval", base::WideToUTF16(L"Обновление, мс"),
                scada::id::Int32, 1000);
  }

  // Historical DB
  {
    CreateObjectType(
        history::id::HistoricalDatabaseType, "HistoricalDatabaseType",
        base::WideToUTF16(L"База данных"), scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates,
                 history::id::HistoricalDatabases,
                 history::id::HistoricalDatabaseType);
    AddProperty(history::id::HistoricalDatabaseType,
                history::id::HistoricalDatabaseType_Depth, {}, "Depth",
                base::WideToUTF16(L"Глубина, дн"), scada::id::Int32, 1);
    AddDataVariable(history::id::HistoricalDatabaseType,
                    history::id::HistoricalDatabaseType_WriteValueDuration,
                    "WriteValueDuration",
                    base::WideToUTF16(L"Задержка записи измерений, мкс"),
                    scada::id::UInt32);
    AddDataVariable(history::id::HistoricalDatabaseType,
                    history::id::HistoricalDatabaseType_PendingTaskCount,
                    "PendingTaskCount",
                    base::WideToUTF16(L"Очередь записи измерений"),
                    scada::id::UInt32);
    AddDataVariable(
        history::id::HistoricalDatabaseType,
        history::id::HistoricalDatabaseType_EventCleanupDuration,
        "EventCleanupDuration",
        base::WideToUTF16(L"Продолжительность очистки событий, мкс"),
        scada::id::UInt32);
    AddDataVariable(
        history::id::HistoricalDatabaseType,
        history::id::HistoricalDatabaseType_ValueCleanupDuration,
        "ValueCleanupDuration",
        base::WideToUTF16(L"Продолжительность очистки измерений, мкс"),
        scada::id::UInt32);
  }

  // System historical database.
  CreateNode(
      {history::id::SystemDatabase,
       scada::NodeClass::Object,
       history::id::HistoricalDatabaseType,
       history::id::HistoricalDatabases,
       scada::id::Organizes,
       scada::NodeAttributes().set_browse_name("System").set_display_name(
           base::WideToUTF16(L"Системная база данных")),
       {{history::id::HistoricalDatabaseType_Depth, 30}}});

  // TsFormat
  {
    CreateObjectType(data_items::id::TsFormatType, "TsFormatType",
                     base::WideToUTF16(L"Формат"), scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates, data_items::id::TsFormats,
                 data_items::id::TsFormatType);
    AddProperty(data_items::id::TsFormatType,
                data_items::id::TsFormatType_OpenLabel, {}, "OpenLabel",
                base::WideToUTF16(L"Подпись 0"), scada::id::LocalizedText,
                scada::LocalizedText{});
    AddProperty(data_items::id::TsFormatType,
                data_items::id::TsFormatType_CloseLabel, {}, "CloseLabel",
                base::WideToUTF16(L"Подпись 1"), scada::id::LocalizedText,
                scada::LocalizedText{});
    AddProperty(data_items::id::TsFormatType,
                data_items::id::TsFormatType_OpenColor, {}, "OpenColor",
                base::WideToUTF16(L"Цвет 0"), scada::id::Int32, 0);
    AddProperty(data_items::id::TsFormatType,
                data_items::id::TsFormatType_CloseColor, {}, "CloseColor",
                base::WideToUTF16(L"Цвет 1"), scada::id::Int32, 0);
  }

  // DataGroup
  {
    CreateObjectType(data_items::id::DataGroupType, "DataGroupType",
                     base::WideToUTF16(L"Группа"), scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates, data_items::id::DataItems,
                 data_items::id::DataGroupType);
    AddReference(address_space_, scada::id::Creates,
                 data_items::id::DataGroupType,
                 data_items::id::DataGroupType);  // recursive
    AddProperty(data_items::id::DataGroupType,
                data_items::id::DataGroupType_Simulated,
                data_items::id::SimulationPropertyCategory, "Simulated",
                base::WideToUTF16(L"Эмуляция"), scada::id::Boolean, false);
  }

  // DataItem
  {
    CreateVariableType(data_items::id::DataItemType, "DataItemType",
                       base::WideToUTF16(L"Объект"), scada::id::BaseDataType,
                       scada::id::BaseVariableType);
    AddReference(address_space_, scada::id::Creates,
                 data_items::id::DataGroupType, data_items::id::DataItemType);
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Alias, {}, "Alias",
                base::WideToUTF16(L"Алиас"), scada::id::String,
                scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Severity, {}, "Severity",
                base::WideToUTF16(L"Важность"), scada::id::Int32, 1);
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Input1,
                data_items::id::ChannelsPropertyCategory, "Input1",
                base::WideToUTF16(L"Основной ввод"), scada::id::String,
                scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Input2,
                data_items::id::ChannelsPropertyCategory, "Input2",
                base::WideToUTF16(L"Резервный ввод"), scada::id::String,
                scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Locked,
                data_items::id::ChannelsPropertyCategory, "Locked",
                base::WideToUTF16(L"Блокировка"), scada::id::Boolean, false);
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_StalePeriod,
                data_items::id::ChannelsPropertyCategory, "StalePeriod",
                base::WideToUTF16(L"Устаревание, с"), scada::id::Int32, 0);
    AddProperty(
        data_items::id::DataItemType, data_items::id::DataItemType_Output,
        data_items::id::ChannelsPropertyCategory, "Output",
        base::WideToUTF16(L"Вывод"), scada::id::String, scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_OutputTwoStaged,
                data_items::id::ChannelsPropertyCategory, "OutputTwoStaged",
                base::WideToUTF16(L"Двухэтапное управление"),
                scada::id::Boolean, true);
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_OutputCondition,
                data_items::id::ChannelsPropertyCategory, "OutputCondition",
                base::WideToUTF16(L"Условие управления"), scada::id::String,
                scada::String{});
    AddProperty(data_items::id::DataItemType,
                data_items::id::DataItemType_Simulated,
                data_items::id::SimulationPropertyCategory, "Simulated",
                base::WideToUTF16(L"Эмуляция"), scada::id::Boolean, false);
    CreateReferenceType(
        data_items::id::DataItemType, data_items::id::HasSimulationSignal,
        data_items::id::SimulationPropertyCategory, "HasSimulationSignal",
        base::WideToUTF16(L"Эмулируемый сигнал"),
        data_items::id::SimulationSignalType);
    CreateReferenceType(
        data_items::id::DataItemType, history::id::HasHistoricalDatabase,
        history::id::HistoryPropertyCategory, "ObjectHistoricalDb",
        base::WideToUTF16(L"База данных"), history::id::HistoricalDatabaseType);
  }

  // DiscreteItem
  {
    CreateVariableType(data_items::id::DiscreteItemType, "DiscreteItemType",
                       base::WideToUTF16(L"Объект ТС"), scada::id::BaseDataType,
                       data_items::id::DataItemType);
    AddProperty(data_items::id::DiscreteItemType,
                data_items::id::DiscreteItemType_Inversion,
                data_items::id::ConversionPropertyCategory, "Inverted",
                base::WideToUTF16(L"Инверсия"), scada::id::Boolean, false);
    CreateReferenceType(
        data_items::id::DiscreteItemType, data_items::id::HasTsFormat,
        data_items::id::DisplayPropertyCategory, "HasTsFormat",
        base::WideToUTF16(L"Параметры"), data_items::id::TsFormatType);
  }

  // AnalogItem
  {
    CreateVariableType(data_items::id::AnalogItemType, "AnalogItemType",
                       base::WideToUTF16(L"Объект ТИТ"),
                       scada::id::BaseDataType, data_items::id::DataItemType);
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_DisplayFormat,
                data_items::id::DisplayPropertyCategory, "DisplayFormat",
                base::WideToUTF16(L"Формат"), scada::id::String, "0.####");
    CreateEnumDataType(
        data_items::id::AnalogConversionDataType,
        data_items::id::AnalogConversionDataType_EnumStrings,
        "AnalogConversionDataType", base::WideToUTF16(L"Преобразование"),
        {base::WideToUTF16(L"Нет"), base::WideToUTF16(L"Линейное")});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_Conversion,
                data_items::id::ConversionPropertyCategory, "Conversion",
                base::WideToUTF16(L"Преобразование"),
                data_items::id::AnalogConversionDataType,
                static_cast<scada::Int32>(0));
    AddProperty(
        data_items::id::AnalogItemType, data_items::id::AnalogItemType_Clamping,
        data_items::id::FilteringPropertyCategory, "Clamping",
        base::WideToUTF16(L"Ограничение диапазона"), scada::id::Boolean, false);
    AddProperty(
        data_items::id::AnalogItemType, data_items::id::AnalogItemType_EuLo,
        data_items::id::ConversionPropertyCategory, "EuLo",
        base::WideToUTF16(L"Логический минимимум"), scada::id::Double, -100.0);
    AddProperty(
        data_items::id::AnalogItemType, data_items::id::AnalogItemType_EuHi,
        data_items::id::ConversionPropertyCategory, "EuHi",
        base::WideToUTF16(L"Логический максимум"), scada::id::Double, 100.0);
    AddProperty(
        data_items::id::AnalogItemType, data_items::id::AnalogItemType_IrLo,
        data_items::id::ConversionPropertyCategory, "IrLo",
        base::WideToUTF16(L"Физический минимимум"), scada::id::Double, 0.0);
    AddProperty(
        data_items::id::AnalogItemType, data_items::id::AnalogItemType_IrHi,
        data_items::id::ConversionPropertyCategory, "IrHi",
        base::WideToUTF16(L"Физический максимум"), scada::id::Double, 65535.0);
    // TODO: Variant.
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_LimitLo,
                data_items::id::LimitsPropertyCategory, "LimitLo",
                base::WideToUTF16(L"Уставка нижняя предаварийная"),
                scada::id::Double, scada::Variant{});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_LimitHi,
                data_items::id::LimitsPropertyCategory, "LimitHi",
                base::WideToUTF16(L"Уставка верхняя предаварийная"),
                scada::id::Double, scada::Variant{});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_LimitLoLo,
                data_items::id::LimitsPropertyCategory, "LimitLoLo",
                base::WideToUTF16(L"Уставка нижняя аварийная"),
                scada::id::Double, scada::Variant{});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_LimitHiHi,
                data_items::id::LimitsPropertyCategory, "LimitHiHi",
                base::WideToUTF16(L"Уставка верхняя аварийная"),
                scada::id::Double, scada::Variant{});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_EngineeringUnits,
                data_items::id::DisplayPropertyCategory, "EngineeringUnits",
                base::WideToUTF16(L"Единицы измерения"),
                scada::id::LocalizedText, scada::LocalizedText{});
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_Aperture,
                data_items::id::FilteringPropertyCategory, "Aperture",
                base::WideToUTF16(L"Апертура"), scada::id::Double, 0.0);
    AddProperty(data_items::id::AnalogItemType,
                data_items::id::AnalogItemType_Deadband,
                data_items::id::FilteringPropertyCategory, "Deadband",
                base::WideToUTF16(L"Мертвая зона"), scada::id::Double, 0.0);
  }

  // Device
  {
    CreateObjectType(devices::id::DeviceType, "DeviceType",
                     base::WideToUTF16(L"Устройство"),
                     scada::id::BaseObjectType);
    AddProperty(devices::id::DeviceType, devices::id::DeviceType_Disabled, {},
                "Disabled", base::WideToUTF16(L"Отключено"), scada::id::Boolean,
                true);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_Online,
                    "Online", base::WideToUTF16(L"Связь"), scada::id::Boolean,
                    false);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_Enabled,
                    "Enabled", base::WideToUTF16(L"Включено"),
                    scada::id::Boolean, false);
    AddDataVariable(devices::id::DeviceType,
                    devices::id::DeviceType_MessagesOut, "MessagesOut",
                    base::WideToUTF16(L"Отправлено сообщений"),
                    scada::id::Int32, 0);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_MessagesIn,
                    "MessagesIn", base::WideToUTF16(L"Принято сообщений"),
                    scada::id::Int32, 0);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_BytesOut,
                    "BytesOut", base::WideToUTF16(L"Отправлено байт"),
                    scada::id::Int32, 0);
    AddDataVariable(devices::id::DeviceType, devices::id::DeviceType_BytesIn,
                    "BytesIn", base::WideToUTF16(L"Принято байт"),
                    scada::id::Int32, 0);
  }

  // Link
  {
    CreateObjectType(devices::id::LinkType, "LinkType",
                     base::WideToUTF16(L"Направление"),
                     devices::id::DeviceType);
    AddReference(address_space_, scada::id::Creates, devices::id::Devices,
                 devices::id::LinkType);
    AddProperty(devices::id::LinkType, devices::id::LinkType_Transport, {},
                "TransportString", base::WideToUTF16(L"Транспорт"),
                scada::id::String, scada::String{});
    AddDataVariable(devices::id::LinkType, devices::id::LinkType_ConnectCount,
                    "ConnectCount", base::WideToUTF16(L"ConnectCount"),
                    scada::id::Int32, 0);
    AddDataVariable(
        devices::id::LinkType, devices::id::LinkType_ActiveConnections,
        "ActiveConnections", base::WideToUTF16(L"ActiveConnections"),
        scada::id::Int32, 0);
  }

  // MODBUS Protocol

  CreateEnumDataType(devices::id::ModbusLinkProtocol,
                     devices::id::ModbusLinkProtocol_EnumStrings,
                     "ModbusLinkProtocol",
                     base::WideToUTF16(L"Тип протокола MODBUS"),
                     {base::WideToUTF16(L"RTU"), base::WideToUTF16(L"ASCII"),
                      base::WideToUTF16(L"TCP")});

  // MODBUS Mode

  CreateEnumDataType(
      devices::id::ModbusModeDataType,
      devices::id::ModbusModeDataType_EnumStrings, "ModbusModeDataType",
      base::WideToUTF16(L"Режим MODBUS"),
      {base::WideToUTF16(L"Опрос"), base::WideToUTF16(L"Ретрансляция")});

  // Modbus Port Device
  {
    CreateObjectType(devices::id::ModbusLinkType, "ModbusLinkType",
                     base::WideToUTF16(L"Направление MODBUS"),
                     devices::id::LinkType);
    AddProperty(devices::id::ModbusLinkType,
                devices::id::ModbusLinkType_Protocol, {}, "Protocol",
                base::WideToUTF16(L"Протокол"), devices::id::ModbusLinkProtocol,
                0);
    AddProperty(devices::id::ModbusLinkType, devices::id::ModbusLinkType_Mode,
                {}, "Mode", base::WideToUTF16(L"Режим"),
                devices::id::ModbusModeDataType, 0);
    AddProperty(devices::id::ModbusLinkType,
                devices::id::ModbusLinkType_RequestDelay, {}, "RequestDelay",
                base::WideToUTF16(L"Задержка запроса, мс"), scada::id::Int32,
                0);
  }

  // Modbus Device
  {
    CreateObjectType(devices::id::ModbusDeviceType, "ModbusDeviceType",
                     base::WideToUTF16(L"Устройство MODBUS"),
                     devices::id::DeviceType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::ModbusLinkType, devices::id::ModbusDeviceType);
    AddProperty(devices::id::ModbusDeviceType,
                devices::id::ModbusDeviceType_Address, {}, "Address",
                base::WideToUTF16(L"Адрес"), scada::id::UInt8, 1);
    AddProperty(devices::id::ModbusDeviceType,
                devices::id::ModbusDeviceType_SendRetryCount, {}, "RepeatCount",
                base::WideToUTF16(L"Попыток повторов передачи"),
                scada::id::Int32, 3);
    AddProperty(devices::id::ModbusDeviceType,
                devices::id::ModbusDeviceType_ResponseTimeout, {},
                "ResponseTimeout", base::WideToUTF16(L"Таймаут ответа, мс"),
                scada::id::Int32, 1000);
  }

  // User
  {
    CreateObjectType(security::id::UserType, "UserType",
                     base::WideToUTF16(L"Пользователь"),
                     scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates, security::id::Users,
                 security::id::UserType);
    AddProperty(security::id::UserType, security::id::UserType_AccessRights, {},
                "AccessRights", base::WideToUTF16(L"Права"), scada::id::Int32,
                0);
  }

  // Root user.
  int root_privileges = (1 << static_cast<int>(scada::Privilege::Configure)) |
                        (1 << static_cast<int>(scada::Privilege::Control));
  CreateNode({
      security::id::RootUser,
      scada::NodeClass::Object,
      security::id::UserType,
      security::id::Users,
      scada::id::Organizes,
      scada::NodeAttributes().set_display_name(base::WideToUTF16(L"root")),
      {{security::id::UserType_AccessRights, root_privileges}},
  });

  // IEC-60870 Protocol

  CreateEnumDataType(devices::id::Iec60870ProtocolDataType,
                     devices::id::Iec60870ProtocolDataType_EnumStrings,
                     "Iec60870ProtocolType",
                     base::WideToUTF16(L"Тип протокола МЭК-60870"),
                     {base::WideToUTF16(L"МЭК-60870-104"),
                      base::WideToUTF16(L"МЭК-60870-101")});

  // IEC-60870 Mode

  CreateEnumDataType(
      devices::id::Iec60870ModeDataType,
      devices::id::Iec60870ModeDataType_EnumStrings, "Iec60870ModeType",
      base::WideToUTF16(L"Режим МЭК-60870"),
      {base::WideToUTF16(L"Опрос"), base::WideToUTF16(L"Ретрансляция"),
       base::WideToUTF16(L"Прослушка")});

  // IEC-60870 Link Device
  {
    CreateObjectType(devices::id::Iec60870LinkType, "Iec60870LinkType",
                     base::WideToUTF16(L"Направление МЭК-60870"),
                     devices::id::LinkType);
    AddProperty(
        devices::id::Iec60870LinkType, devices::id::Iec60870LinkType_Protocol,
        {}, "Protocol", base::WideToUTF16(L"Протокол"),
        devices::id::Iec60870ProtocolDataType, static_cast<scada::Int32>(0));
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_Mode, {}, "Mode",
                base::WideToUTF16(L"Режим"), devices::id::Iec60870ModeDataType,
                static_cast<scada::Int32>(0));
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_SendQueueSize, {},
                "SendQueueSize", base::WideToUTF16(L"Очередь передачи (K)"),
                scada::id::Int32, 12);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_ReceiveQueueSize, {},
                "ReceiveQueueSize", base::WideToUTF16(L"Очередь приема (W)"),
                scada::id::Int32, 8);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_ConnectTimeout, {},
                "ConnectTimeout", base::WideToUTF16(L"Таймаут связи (T0), с"),
                scada::id::Int32, 30);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_ConfirmationTimeout, {},
                "ConfirmationTimeout",
                base::WideToUTF16(L"Таймаут подтверждения, с"),
                scada::id::Int32, 5);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_TerminationTimeout, {},
                "TerminationTimeout", base::WideToUTF16(L"Таймаут операции, с"),
                scada::id::Int32, 20);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_DeviceAddressSize, {},
                "DeviceAddressSize",
                base::WideToUTF16(L"Размер адреса устройства"),
                scada::id::UInt8, 2);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_COTSize, {}, "CotSize",
                base::WideToUTF16(L"Размер причины передачи"), scada::id::UInt8,
                2);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_InfoAddressSize, {},
                "InfoAddressSize", base::WideToUTF16(L"Размер адреса объекта"),
                scada::id::UInt8, 3);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_DataCollection, {}, "CollectData",
                base::WideToUTF16(L"Сбор данных"), scada::id::Boolean, true);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_SendRetryCount, {}, "SendRetries",
                base::WideToUTF16(L"Попыток повторов передачи"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_CRCProtection, {},
                "CrcProtection", base::WideToUTF16(L"Защита CRC"),
                scada::id::Boolean, false);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_SendTimeout, {}, "SendTimeout",
                base::WideToUTF16(L"Таймаут передачи (T1), с"),
                scada::id::Int32, 15);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_ReceiveTimeout, {},
                "ReceiveTimeout", base::WideToUTF16(L"Таймаут приема (T2), с"),
                scada::id::Int32, 10);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_IdleTimeout, {}, "IdleTimeout",
                base::WideToUTF16(L"Таймаут простоя (T3), с"), scada::id::Int32,
                20);
    AddProperty(devices::id::Iec60870LinkType,
                devices::id::Iec60870LinkType_AnonymousMode, {},
                "AnonymousMode", base::WideToUTF16(L"Анонимный режим"),
                scada::id::Boolean, false);
  }

  // IEC-60870 Device
  {
    CreateObjectType(devices::id::Iec60870DeviceType, "Iec60870DeviceType",
                     base::WideToUTF16(L"Устройство МЭК-60870"),
                     devices::id::DeviceType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::Iec60870LinkType,
                 devices::id::Iec60870DeviceType);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_Address, {}, "Address",
                base::WideToUTF16(L"Адрес"), scada::id::UInt16, 1);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_LinkAddress, {}, "LinkAddress",
                base::WideToUTF16(L"Адрес коммутатора"), scada::id::UInt16, 1);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_StartupInterrogation, {},
                "InterrogateOnStart",
                base::WideToUTF16(L"Полный опрос при запуске"),
                scada::id::Boolean, true);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriod, {},
                "InterrogationPeriod",
                base::WideToUTF16(L"Период полного опроса, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_StartupClockSync, {},
                "SynchronizeClockOnStart",
                base::WideToUTF16(L"Синхронизация часов при запуске"),
                scada::id::Boolean, false);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_ClockSyncPeriod, {},
                "ClockSynchronizationPeriod",
                base::WideToUTF16(L"Период синхронизации часов, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_UtcTime, {}, "UtcTime",
                base::WideToUTF16(L"Время UTC"), scada::id::Boolean, false);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup1, {},
                "Group1PollPeriod",
                base::WideToUTF16(L"Период опроса группы 1, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup2, {},
                "Group2PollPeriod",
                base::WideToUTF16(L"Период опроса группы 2, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup3, {},
                "Group3PollPeriod",
                base::WideToUTF16(L"Период опроса группы 3, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup4, {},
                "Group4PollPeriod",
                base::WideToUTF16(L"Период опроса группы 4, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup5, {},
                "Group5PollPeriod",
                base::WideToUTF16(L"Период опроса группы 5, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup6, {},
                "Group6PollPeriod",
                base::WideToUTF16(L"Период опроса группы 6, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup7, {},
                "Group7PollPeriod",
                base::WideToUTF16(L"Период опроса группы 7, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup8, {},
                "Group8PollPeriod",
                base::WideToUTF16(L"Период опроса группы 8, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup9, {},
                "Group9PollPeriod",
                base::WideToUTF16(L"Период опроса группы 9, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup10, {},
                "Group10PollPeriod",
                base::WideToUTF16(L"Период опроса группы 10, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup11, {},
                "Group11PollPeriod",
                base::WideToUTF16(L"Период опроса группы 11, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup12, {},
                "Group12PollPeriod",
                base::WideToUTF16(L"Период опроса группы 12, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup13, {},
                "Group13PollPeriod",
                base::WideToUTF16(L"Период опроса группы 13, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup14, {},
                "Group14PollPeriod",
                base::WideToUTF16(L"Период опроса группы 14, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup15, {},
                "Group15PollPeriod",
                base::WideToUTF16(L"Период опроса группы 15, с"),
                scada::id::Int32, 0);
    AddProperty(devices::id::Iec60870DeviceType,
                devices::id::Iec60870DeviceType_InterrogationPeriodGroup16, {},
                "Group16PollPeriod",
                base::WideToUTF16(L"Период опроса группы 16, с"),
                scada::id::Int32, 0);
  }

  // IEC-61850 Logical Node
  {
    CreateObjectType(devices::id::Iec61850LogicalNodeType,
                     "Iec61850LogicalNodeType",
                     base::WideToUTF16(L"Логический узел МЭК-61850"),
                     scada::id::BaseObjectType);
  }

  // IEC-61850 Data Variable
  {
    CreateVariableType(devices::id::Iec61850DataVariableType,
                       "Iec61850DataVariableType",
                       base::WideToUTF16(L"Объект данных МЭК-61850"),
                       scada::id::BaseDataType, scada::id::BaseVariableType);
  }

  // IEC-61850 Control Object
  {
    CreateObjectType(devices::id::Iec61850ControlObjectType,
                     "Iec61850ControlObjectType",
                     base::WideToUTF16(L"Объект управления МЭК-61850"),
                     scada::id::BaseObjectType);
  }

  // IEC-61850 Device
  {
    CreateObjectType(devices::id::Iec61850DeviceType, "Iec61850DeviceType",
                     base::WideToUTF16(L"Устройство МЭК-61850"),
                     devices::id::DeviceType);
    AddReference(address_space_, scada::id::Creates, devices::id::Devices,
                 devices::id::Iec61850DeviceType);
    {
      auto model = std::make_unique<scada::GenericObject>();
      model->set_id(devices::id::Iec61850DeviceType_Model);
      model->SetBrowseName("Model");
      model->SetDisplayName(base::WideToUTF16(L"Модель"));
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
                devices::id::Iec61850DeviceType_Host, {}, "Host",
                base::WideToUTF16(L"Хост"), scada::id::String, scada::String{});
    AddProperty(devices::id::Iec61850DeviceType,
                devices::id::Iec61850DeviceType_Port, {}, "Port",
                base::WideToUTF16(L"Порт"), scada::id::Int32, 102);
  }

  // IEC-61850 ConfigurableObject
  {
    CreateObjectType(
        devices::id::Iec61850ConfigurableObjectType, "ConfigurableObject",
        base::WideToUTF16(L"Параметр МЭК-61850"), scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::Iec61850ConfigurableObjectType,
                 devices::id::Iec61850ConfigurableObjectType);
    AddProperty(devices::id::Iec61850ConfigurableObjectType,
                devices::id::Iec61850ConfigurableObjectType_Reference, {},
                "Address", base::WideToUTF16(L"Адрес"), scada::id::String,
                scada::String{});
  }

  // IEC-61850 RCB
  {
    CreateObjectType(devices::id::Iec61850RcbType, "RCB",
                     base::WideToUTF16(L"RCB МЭК-61850"),
                     devices::id::Iec61850ConfigurableObjectType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::Iec61850DeviceType, devices::id::Iec61850RcbType);
  }

  // IEC Retransmission Item
  {
    CreateObjectType(devices::id::TransmissionItemType, "TransmissionItemType",
                     base::WideToUTF16(L"Ретрансляция"),
                     scada::id::BaseObjectType);
    AddReference(address_space_, scada::id::Creates,
                 devices::id::TransmissionItems,
                 devices::id::TransmissionItemType);
    CreateReferenceType(
        devices::id::TransmissionItemType, devices::id::HasTransmissionSource,
        {}, "IecTransmitSource", base::WideToUTF16(L"Объект источник"),
        scada::id::BaseVariableType);
    CreateReferenceType(
        devices::id::TransmissionItemType, devices::id::HasTransmissionTarget,
        {}, "IecTransmitTargetDevice",
        base::WideToUTF16(L"Устройство приемник"), devices::id::DeviceType);
    AddProperty(devices::id::TransmissionItemType,
                devices::id::TransmissionItemType_SourceAddress, {},
                "InfoAddress", base::WideToUTF16(L"Адрес объекта приемника"),
                scada::id::Int32, 1);
  }

  // Aliases
  {
    CreateNode(
        {data_items::id::Aliases, scada::NodeClass::Object,
         scada::id::FolderType, scada::id::ObjectsFolder, scada::id::Organizes,
         scada::NodeAttributes{}.set_browse_name("Aliases").set_display_name(
             base::WideToUTF16(L"Алиасы"))});

    CreateObjectType(data_items::id::AliasType, "AliasType",
                     base::WideToUTF16(L"Алиас"), scada::id::BaseObjectType);
    CreateReferenceType(data_items::id::AliasOf, "AliasOf", {},
                        scada::id::NonHierarchicalReferences);
  }

  CreateFileSystemAddressSpace();

  // OPC
  {
    CreateNode({opc::id::OPC, scada::NodeClass::Object, scada::id::FolderType,
                scada::id::ObjectsFolder, scada::id::Organizes,
                scada::NodeAttributes{}.set_browse_name("OPC").set_display_name(
                    base::WideToUTF16(L"OPC"))});
  }

  CreateReferenceType(data_items::id::DataGroupType, data_items::id::HasDevice,
                      data_items::id::ChannelsPropertyCategory, "HasDevice",
                      base::WideToUTF16(L"Устройство"),
                      devices::id::DeviceType);
}
