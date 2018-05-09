#pragma once

#include "base/strings/utf_string_conversions.h"
#include "core/standard_node_ids.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"

struct StandardAddressSpace;

class GenericDataVariable : public scada::GenericVariable {
 public:
  GenericDataVariable(StandardAddressSpace& std, scada::NodeId id, scada::QualifiedName browse_name,
      scada::LocalizedText display_name, scada::VariableType& variable_type, const scada::DataType& data_type,
      scada::Variant default_value = scada::Variant());
};

class GenericProperty : public scada::GenericVariable {
 public:
  GenericProperty(StandardAddressSpace& std, scada::NodeId id, scada::QualifiedName browse_name,
      scada::LocalizedText display_name, const scada::DataType& data_type,
      scada::Variant default_value = scada::Variant());
};

struct StandardAddressSpace {
  StandardAddressSpace();

  scada::ReferenceType HierarchicalReference{scada::id::HierarchicalReferences, "HierarchicalReference", base::WideToUTF16(L"HierarchicalReference")};
  scada::ReferenceType NonHierarchicalReference{scada::id::NonHierarchicalReferences, "NonHierarchicalReference", base::WideToUTF16(L"NonHierarchicalReference")};
  scada::ReferenceType Aggregates{scada::id::Aggregates, "Aggregates", base::WideToUTF16(L"Aggregates")};
  scada::ReferenceType HasProperty{scada::id::HasProperty, "HasProperty", base::WideToUTF16(L"HasProperty")};
  scada::ReferenceType HasComponent{scada::id::HasComponent, "HasComponent", base::WideToUTF16(L"HasComponent")};
  scada::ReferenceType HasSubtype{scada::id::HasSubtype, "HasSubtype", base::WideToUTF16(L"HasSubtype")};
  scada::ReferenceType HasTypeDefinition{scada::id::HasTypeDefinition, "HasTypeDefinition", base::WideToUTF16(L"HasTypeDefinition")};
  scada::ReferenceType HasModellingRule{scada::id::HasModellingRule, "HasModellingRule",base::WideToUTF16( L"HasModellingRule")};
  scada::ReferenceType Organizes{scada::id::Organizes, "Organizes", base::WideToUTF16(L"Organizes")};

  scada::DataType BaseDataType{scada::id::BaseDataType, "BaseDataType", base::WideToUTF16(L"BaseDataType")};
  scada::DataType BoolDataType{scada::id::Boolean, "Bool", base::WideToUTF16(L"Bool")};
  scada::DataType IntDataType{scada::id::Int32, "Int", base::WideToUTF16(L"Int")};
  scada::DataType DoubleDataType{scada::id::Double, "Double", base::WideToUTF16(L"Double")};
  scada::DataType StringDataType{scada::id::String, "String", base::WideToUTF16(L"String")};
  scada::DataType LocalizedTextDataType{scada::id::LocalizedText, "LocalizedText", base::WideToUTF16(L"LocalizedText")};
  scada::DataType NodeIdDataType{scada::id::NodeId, "NodeId", base::WideToUTF16(L"NodeId")};

  scada::ObjectType BaseObjectType{scada::id::BaseObjectType, "BaseObjectType", base::WideToUTF16(L"BaseObjectType")};
  scada::ObjectType ModellingRule_Mandatory{scada::id::ModellingRule_Mandatory, "Mandatory", base::WideToUTF16(L"Mandatory")};
  scada::ObjectType FolderType{scada::id::FolderType, "FolderType", base::WideToUTF16(L"FolderType")};

  scada::VariableType BaseVariableType{scada::id::BaseVariableType, "BaseVariableType", base::WideToUTF16(L"BaseVariableType"), BaseDataType};
  scada::VariableType PropertyType{scada::id::PropertyType, "PropertyType", base::WideToUTF16(L"PropertyType"), BaseDataType};
};
