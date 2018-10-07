#include "address_space/standard_address_space.h"

#include "address_space/address_space_impl.h"
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

StandardAddressSpace::StandardAddressSpace(AddressSpaceImpl& address_space)
    : RootFolder{scada::id::RootFolder, "RootFolder",
                 base::WideToUTF16(L"Корневая папка")},
      ObjectsFolder{scada::id::ObjectsFolder, "ObjectsFolder",
                    base::WideToUTF16(L"Объекты")},
      TypesFolder{scada::id::TypesFolder, "TypesFolder",
                  base::WideToUTF16(L"Типы")} {
  address_space.AddNode(RootFolder);
  address_space.AddNode(ObjectsFolder);
  address_space.AddNode(TypesFolder);

  address_space.AddNode(References);
  address_space.AddNode(HierarchicalReference);
  address_space.AddNode(NonHierarchicalReference);
  address_space.AddNode(Aggregates);
  address_space.AddNode(HasProperty);
  address_space.AddNode(HasComponent);
  address_space.AddNode(HasSubtype);
  address_space.AddNode(HasTypeDefinition);
  address_space.AddNode(HasModellingRule);
  address_space.AddNode(Organizes);

  address_space.AddNode(BaseDataType);
  address_space.AddNode(BoolDataType);
  address_space.AddNode(Int32DataType);
  address_space.AddNode(UInt32DataType);
  address_space.AddNode(Int64DataType);
  address_space.AddNode(UInt64DataType);
  address_space.AddNode(DoubleDataType);
  address_space.AddNode(StringDataType);
  address_space.AddNode(LocalizedTextDataType);
  address_space.AddNode(NodeIdDataType);
  address_space.AddNode(ByteStringDataType);

  address_space.AddNode(BaseObjectType);
  address_space.AddNode(FolderType);

  address_space.AddNode(BaseVariableType);
  address_space.AddNode(PropertyType);

  address_space.AddNode(ModellingRules);
  address_space.AddNode(ModellingRule_Mandatory);

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
  scada::AddReference(HasSubtype, BaseDataType, Int32DataType);
  scada::AddReference(HasSubtype, BaseDataType, UInt32DataType);
  scada::AddReference(HasSubtype, BaseDataType, Int64DataType);
  scada::AddReference(HasSubtype, BaseDataType, UInt64DataType);
  scada::AddReference(HasSubtype, BaseDataType, DoubleDataType);
  scada::AddReference(HasSubtype, BaseDataType, StringDataType);
  scada::AddReference(HasSubtype, BaseDataType, LocalizedTextDataType);
  scada::AddReference(HasSubtype, BaseDataType, NodeIdDataType);
  scada::AddReference(HasSubtype, BaseDataType, ByteStringDataType);

  // BaseObjectType
  scada::AddReference(Organizes, TypesFolder, BaseObjectType);

  scada::AddReference(HasSubtype, BaseObjectType, FolderType);

  // BaseVariableType
  scada::AddReference(Organizes, TypesFolder, BaseVariableType);
  scada::AddReference(HasSubtype, BaseVariableType, PropertyType);

  // ModellingRules
  scada::AddReference(HasTypeDefinition, ModellingRules, FolderType);
  scada::AddReference(Organizes, ObjectsFolder, ModellingRules);
  scada::AddReference(HasTypeDefinition, ModellingRule_Mandatory,
                      BaseObjectType);
  scada::AddReference(Organizes, ModellingRules, ModellingRule_Mandatory);
}
