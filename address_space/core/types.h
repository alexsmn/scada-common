#pragma once

#include "core/configuration_types.h"
#include "core/node_id.h"
#include "core/standard_node_ids.h"
#include "core/type_definition.h"
#include "core/variable.h"
#include "common/scada_node_ids.h"

class ConfigurationImpl;

struct StandardAddressSpace {
  StandardAddressSpace();

  scada::ReferenceType HierarchicalReference{scada::id::HierarchicalReferences, "HierarchicalReference", L"HierarchicalReference"};
  scada::ReferenceType NonHierarchicalReference{scada::id::NonHierarchicalReferences, "NonHierarchicalReference", L"NonHierarchicalReference"};
  scada::ReferenceType Aggregates{scada::id::Aggregates, "Aggregates", L"Aggregates"};
  scada::ReferenceType HasProperty{scada::id::HasProperty, "HasProperty", L"HasProperty"};
  scada::ReferenceType HasComponent{scada::id::HasComponent, "HasComponent", L"HasComponent"};
  scada::ReferenceType HasSubtype{scada::id::HasSubtype, "HasSubtype", L"HasSubtype"};
  scada::ReferenceType HasTypeDefinition{scada::id::HasTypeDefinition, "HasTypeDefinition", L"HasTypeDefinition"};
  scada::ReferenceType HasModellingRule{scada::id::HasModellingRule, "HasModellingRule", L"HasModellingRule"};
  scada::ReferenceType Organizes{scada::id::Organizes, "Organizes", L"Organizes"};

  scada::DataType BaseDataType{scada::id::BaseDataType, "BaseDataType", L"BaseDataType"};
  scada::DataType BoolDataType{scada::id::Boolean, "Bool", L"Bool"};
  scada::DataType IntDataType{scada::id::Int32, "Int", L"Int"};
  scada::DataType DoubleDataType{scada::id::Double, "Double", L"Double"};
  scada::DataType StringDataType{scada::id::String, "String", L"String"};
  scada::DataType LocalizedTextDataType{scada::id::LocalizedText, "LocalizedText", L"LocalizedText"};
  scada::DataType NodeIdDataType{scada::id::NodeId, "NodeId", L"NodeId"};

  scada::ObjectType BaseObjectType{scada::id::BaseObjectType, "BaseObjectType", L"BaseObjectType"};
  scada::ObjectType ModellingRule_Mandatory{scada::id::ModellingRule_Mandatory, "Mandatory", L"Mandatory"};
  scada::ObjectType FolderType{scada::id::FolderType, "FolderType", L"FolderType"};

  scada::VariableType BaseVariableType{scada::id::BaseVariableType, "BaseVariableType", L"BaseVariableType", BaseDataType};
  scada::VariableType PropertyType{scada::id::PropertyType, "PropertyType", L"PropertyType", BaseDataType};
};

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

struct StaticAddressSpace {
  explicit StaticAddressSpace(StandardAddressSpace& std);

  struct DeviceType : public scada::ObjectType {
    DeviceType(StandardAddressSpace& std, scada::NodeId id, scada::QualifiedName browse_name,
        scada::LocalizedText display_name);

    GenericProperty Disabled;
    GenericDataVariable Online;
    GenericDataVariable Enabled;
  };

  DeviceType DeviceType;
};

void CreateCommonTypes(ConfigurationImpl& configuration);

enum TreeID { kTreeObjects,
              kTreeDevices,
              kTreeUser,
              kTreeTsFormat,
              kTreeSimulator,
              kTreeHistoricalDB,
              kTreeIecTransmit,
              kTreeCount };

struct TreeDef {
  scada::NodeId node_id;
  scada::NodeId type_id;
  base::StringPiece16 title;
};

extern const TreeDef kTreeDefs[];
