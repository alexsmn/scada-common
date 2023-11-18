#include "address_space/standard_address_space.h"

#include "address_space/address_space_impl.h"

GenericDataVariable::GenericDataVariable(StandardAddressSpace& std,
                                         AddressSpaceImpl& address_space,
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
  address_space.AddReference(std.HasTypeDefinition, *this, variable_type);
  address_space.AddReference(std.HasModellingRule, *this,
                             std.ModellingRule_Mandatory);
}

GenericProperty::GenericProperty(StandardAddressSpace& std,
                                 AddressSpaceImpl& address_space,
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
  address_space.AddReference(std.HasTypeDefinition, *this, std.PropertyType);
  address_space.AddReference(std.HasModellingRule, *this,
                             std.ModellingRule_Mandatory);
}

StandardAddressSpace::StandardAddressSpace(AddressSpaceImpl& address_space)
    : RootFolder{scada::id::RootFolder, "RootFolder", u"Корневая папка"},
      ObjectsFolder{scada::id::ObjectsFolder, "ObjectsFolder", u"Объекты"},
      TypesFolder{scada::id::TypesFolder, "TypesFolder", u"Типы"},
      Server{scada::id::Server, "Server", u"Сервер"} {
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
  address_space.AddNode(HasEventSource);
  address_space.AddNode(HasNotifier);
  address_space.AddNode(Organizes);

  address_space.AddNode(BaseDataType);
  address_space.AddNode(BoolDataType);
  address_space.AddNode(Int8DataType);
  address_space.AddNode(UInt8DataType);
  address_space.AddNode(Int16DataType);
  address_space.AddNode(UInt16DataType);
  address_space.AddNode(Int32DataType);
  address_space.AddNode(UInt32DataType);
  address_space.AddNode(Int64DataType);
  address_space.AddNode(UInt64DataType);
  address_space.AddNode(DoubleDataType);
  address_space.AddNode(StringDataType);
  address_space.AddNode(LocalizedTextDataType);
  address_space.AddNode(NodeIdDataType);
  address_space.AddNode(ByteStringDataType);
  address_space.AddNode(DateTimeDataType);
  address_space.AddNode(EnumerationDataType);

  address_space.AddNode(BaseObjectType);
  address_space.AddNode(FolderType);

  address_space.AddNode(BaseVariableType);
  address_space.AddNode(PropertyType);

  address_space.AddNode(ModellingRules);
  address_space.AddNode(ModellingRule_Mandatory);

  address_space.AddNode(Server);

  address_space.AddNode(AcknowledgeableConditionType_Acknowledge);

  address_space.AddReference(HasTypeDefinition, RootFolder, FolderType);
  /*address_space.AddReference(HasMethod, AcknowledgeableConditionType,
                             AcknowledgeableConditionType_Acknowledge);*/

  // ObjectsFolder
  address_space.AddReference(HasTypeDefinition, ObjectsFolder, FolderType);
  address_space.AddReference(Organizes, RootFolder, ObjectsFolder);

  // TypesFolder
  address_space.AddReference(HasTypeDefinition, TypesFolder, FolderType);
  address_space.AddReference(Organizes, RootFolder, TypesFolder);

  // References
  address_space.AddReference(Organizes, TypesFolder, References);

  address_space.AddReference(HasSubtype, References, HierarchicalReference);
  address_space.AddReference(HasSubtype, HierarchicalReference, Aggregates);
  address_space.AddReference(HasSubtype, HierarchicalReference, Organizes);
  address_space.AddReference(HasSubtype, HierarchicalReference, HasSubtype);
  address_space.AddReference(HasSubtype, Aggregates, HasProperty);
  address_space.AddReference(HasSubtype, Aggregates, HasComponent);

  address_space.AddReference(HasSubtype, References, NonHierarchicalReference);
  address_space.AddReference(HasSubtype, NonHierarchicalReference,
                             HasTypeDefinition);
  address_space.AddReference(HasSubtype, NonHierarchicalReference,
                             HasModellingRule);
  address_space.AddReference(HasSubtype, HierarchicalReference, HasEventSource);
  address_space.AddReference(HasSubtype, HasEventSource, HasNotifier);

  // BaseDataType
  address_space.AddReference(Organizes, TypesFolder, BaseDataType);

  address_space.AddReference(HasSubtype, BaseDataType, BoolDataType);
  address_space.AddReference(HasSubtype, BaseDataType, Int8DataType);
  address_space.AddReference(HasSubtype, BaseDataType, UInt8DataType);
  address_space.AddReference(HasSubtype, BaseDataType, Int16DataType);
  address_space.AddReference(HasSubtype, BaseDataType, UInt16DataType);
  address_space.AddReference(HasSubtype, BaseDataType, Int32DataType);
  address_space.AddReference(HasSubtype, BaseDataType, UInt32DataType);
  address_space.AddReference(HasSubtype, BaseDataType, Int64DataType);
  address_space.AddReference(HasSubtype, BaseDataType, UInt64DataType);
  address_space.AddReference(HasSubtype, BaseDataType, DoubleDataType);
  address_space.AddReference(HasSubtype, BaseDataType, StringDataType);
  address_space.AddReference(HasSubtype, BaseDataType, LocalizedTextDataType);
  address_space.AddReference(HasSubtype, BaseDataType, NodeIdDataType);
  address_space.AddReference(HasSubtype, BaseDataType, ByteStringDataType);
  address_space.AddReference(HasSubtype, BaseDataType, DateTimeDataType);
  address_space.AddReference(HasSubtype, BaseDataType, EnumerationDataType);

  // BaseObjectType
  address_space.AddReference(Organizes, TypesFolder, BaseObjectType);

  address_space.AddReference(HasSubtype, BaseObjectType, FolderType);

  // BaseVariableType
  address_space.AddReference(Organizes, TypesFolder, BaseVariableType);
  address_space.AddReference(HasSubtype, BaseVariableType, PropertyType);

  // ModellingRules
  address_space.AddReference(HasTypeDefinition, ModellingRules, FolderType);
  address_space.AddReference(Organizes, ObjectsFolder, ModellingRules);
  address_space.AddReference(HasTypeDefinition, ModellingRule_Mandatory,
                             BaseObjectType);
  address_space.AddReference(Organizes, ModellingRules,
                             ModellingRule_Mandatory);

  // Server
  address_space.AddReference(Organizes, ObjectsFolder, Server);
  address_space.AddReference(HasTypeDefinition, Server, BaseObjectType);
}
