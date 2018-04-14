#include "core/standard_address_space.h"

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
