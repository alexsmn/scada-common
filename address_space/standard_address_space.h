#pragma once

#include "address_space/folder.h"
#include "address_space/method.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "scada/standard_node_ids.h"

class AddressSpaceImpl;
struct StandardAddressSpace;

class GenericDataVariable : public scada::GenericVariable {
 public:
  GenericDataVariable(StandardAddressSpace& std,
                      AddressSpaceImpl& address_space,
                      scada::NodeId id,
                      scada::QualifiedName browse_name,
                      scada::LocalizedText display_name,
                      scada::VariableType& variable_type,
                      const scada::DataType& data_type,
                      scada::Variant default_value = scada::Variant());
};

class GenericProperty : public scada::GenericVariable {
 public:
  GenericProperty(StandardAddressSpace& std,
                  AddressSpaceImpl& address_space,
                  scada::NodeId id,
                  scada::QualifiedName browse_name,
                  scada::LocalizedText display_name,
                  const scada::DataType& data_type,
                  scada::Variant default_value = scada::Variant());
};

struct StandardAddressSpace {
  explicit StandardAddressSpace(AddressSpaceImpl& address_space);

  scada::Folder RootFolder;
  scada::Folder ObjectsFolder;
  scada::Folder TypesFolder;

  scada::ReferenceType References{scada::id::References, "References"};
  scada::ReferenceType HierarchicalReference{
      scada::id::HierarchicalReferences,
      "HierarchicalReference",
  };
  scada::ReferenceType NonHierarchicalReference{
      scada::id::NonHierarchicalReferences,
      "NonHierarchicalReference",
      {}};
  scada::ReferenceType Aggregates{scada::id::Aggregates, "Aggregates"};
  scada::ReferenceType HasProperty{scada::id::HasProperty, "HasProperty"};
  scada::ReferenceType HasComponent{scada::id::HasComponent, "HasComponent"};
  scada::ReferenceType HasSubtype{scada::id::HasSubtype, "HasSubtype"};
  scada::ReferenceType HasTypeDefinition{scada::id::HasTypeDefinition,
                                         "HasTypeDefinition"};
  scada::ReferenceType HasModellingRule{scada::id::HasModellingRule,
                                        "HasModellingRule"};
  scada::ReferenceType HasEventSource{scada::id::HasEventSource,
                                      "HasEventSource"};
  scada::ReferenceType HasNotifier{scada::id::HasNotifier, "HasNotifier"};
  scada::ReferenceType Organizes{scada::id::Organizes, "Organizes"};

  scada::DataType BaseDataType{scada::id::BaseDataType, "BaseDataType", {}};
  scada::DataType BoolDataType{scada::id::Boolean, "Bool", {}};
  scada::DataType Int8DataType{scada::id::Int8, "Int8", {}};
  scada::DataType UInt8DataType{scada::id::UInt8, "UInt8", {}};
  scada::DataType Int16DataType{scada::id::Int16, "Int16", {}};
  scada::DataType UInt16DataType{scada::id::UInt16, "UInt16", {}};
  scada::DataType Int32DataType{scada::id::Int32, "Int32", {}};
  scada::DataType UInt32DataType{scada::id::UInt32, "UInt32", {}};
  scada::DataType Int64DataType{scada::id::Int64, "Int64", {}};
  scada::DataType UInt64DataType{scada::id::UInt64, "UInt64", {}};
  scada::DataType DoubleDataType{scada::id::Double, "Double", {}};
  scada::DataType StringDataType{scada::id::String, "String", {}};
  scada::DataType LocalizedTextDataType{scada::id::LocalizedText,
                                        "LocalizedText",
                                        {}};
  scada::DataType NodeIdDataType{scada::id::NodeId, "NodeId", {}};
  scada::DataType ByteStringDataType{scada::id::ByteString, "ByteString", {}};
  scada::DataType DateTimeDataType{scada::id::DateTime, "DateTime", {}};
  scada::DataType EnumerationDataType{scada::id::Enumeration,
                                      "Enumeration",
                                      {}};

  scada::ObjectType BaseObjectType{scada::id::BaseObjectType,
                                   "BaseObjectType",
                                   {}};
  scada::ObjectType FolderType{scada::id::FolderType, "FolderType", {}};

  scada::Folder ModellingRules{scada::id::ModellingRules, "ModellingRules", {}};
  scada::GenericObject ModellingRule_Mandatory{
      scada::id::ModellingRule_Mandatory,
      "Mandatory",
      {}};

  scada::VariableType BaseVariableType{scada::id::BaseVariableType,
                                       "BaseVariableType",
                                       {},
                                       BaseDataType};
  scada::VariableType PropertyType{scada::id::PropertyType,
                                   "PropertyType",
                                   {},
                                   BaseDataType};

  scada::GenericObject Server;

  scada::GenericMethod AcknowledgeableConditionType_Acknowledge{
      scada::id::AcknowledgeableConditionType_Acknowledge,
      "Acknowledge",
      {}};
};
