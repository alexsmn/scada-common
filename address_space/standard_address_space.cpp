#include "address_space/standard_address_space.h"

#include "base/strings/utf_string_conversions.h"

GenericDataVariable::GenericDataVariable(StandardAddressSpace& std,
                                         scada::NodeId id,
                                         scada::QualifiedName browse_name,
                                         scada::LocalizedText display_name,
                                         scada::VariableType& variable_type,
                                         const scada::DataType& data_type,
                                         scada::Variant default_value)
    : GenericVariable(std::move(id),
                      std::move(browse_name),
                      std::move(display_name),
                      data_type,
                      std::move(default_value)) {
  scada::AddReference(std.HasTypeDefinition, *this, variable_type);
  scada::AddReference(std.HasModellingRule, *this, std.ModellingRule_Mandatory);
}

GenericProperty::GenericProperty(StandardAddressSpace& std,
                                 scada::NodeId id,
                                 scada::QualifiedName browse_name,
                                 scada::LocalizedText display_name,
                                 const scada::DataType& data_type,
                                 scada::Variant default_value)
    : GenericVariable(std::move(id),
                      std::move(browse_name),
                      std::move(display_name),
                      data_type,
                      std::move(default_value)) {
  scada::AddReference(std.HasTypeDefinition, *this, std.PropertyType);
  scada::AddReference(std.HasModellingRule, *this, std.ModellingRule_Mandatory);
}

StandardAddressSpace::StandardAddressSpace()
    : RootFolder{scada::id::RootFolder, "RootFolder",
                 base::WideToUTF16(L"Корневая папка")},
      ObjectsFolder{scada::id::ObjectsFolder, "ObjectsFolder",
                    base::WideToUTF16(L"Объекты")},
      TypesFolder{scada::id::TypesFolder, "TypesFolder",
                  base::WideToUTF16(L"Типы")} {
  scada::AddReference(HasTypeDefinition, RootFolder, FolderType);

  // ObjectsFolder
  scada::AddReference(HasTypeDefinition, ObjectsFolder, FolderType);
  scada::AddReference(Organizes, RootFolder, ObjectsFolder);

  // TypesFolder
  scada::AddReference(HasTypeDefinition, TypesFolder, FolderType);
  scada::AddReference(Organizes, RootFolder, TypesFolder);

  // References
  scada::AddReference(Organizes, TypesFolder, References);

  scada::AddReference(HasSubtype, References, HierarchicalReference);
  scada::AddReference(HasSubtype, HierarchicalReference, Aggregates);
  scada::AddReference(HasSubtype, HierarchicalReference, Organizes);
  scada::AddReference(HasSubtype, Aggregates, HasProperty);
  scada::AddReference(HasSubtype, Aggregates, HasComponent);
  scada::AddReference(HasSubtype, Aggregates, HasSubtype);

  scada::AddReference(HasSubtype, References, NonHierarchicalReference);
  scada::AddReference(HasSubtype, NonHierarchicalReference, HasTypeDefinition);
  scada::AddReference(HasSubtype, NonHierarchicalReference, HasModellingRule);

  // BaseDataType
  scada::AddReference(Organizes, TypesFolder, BaseDataType);

  scada::AddReference(HasSubtype, BaseDataType, BoolDataType);
  scada::AddReference(HasSubtype, BaseDataType, IntDataType);
  scada::AddReference(HasSubtype, BaseDataType, DoubleDataType);
  scada::AddReference(HasSubtype, BaseDataType, StringDataType);
  scada::AddReference(HasSubtype, BaseDataType, LocalizedTextDataType);
  scada::AddReference(HasSubtype, BaseDataType, NodeIdDataType);

  // BaseObjectType
  scada::AddReference(Organizes, TypesFolder, BaseObjectType);

  scada::AddReference(HasSubtype, ModellingRule_Mandatory, BaseObjectType);
  scada::AddReference(HasSubtype, BaseObjectType, FolderType);

  // BaseVariableType
  scada::AddReference(Organizes, TypesFolder, BaseVariableType);
  scada::AddReference(HasSubtype, BaseVariableType, PropertyType);
}
