#pragma once

#include "scada/variant.h"

namespace scada {
class DataType;
class Node;
class ObjectType;
class ReferenceType;
class Variable;
class VariableType;
struct NodeState;
}  // namespace scada

class AddressSpaceImpl;
class NodeFactory;

class ScadaAddressSpaceBuilder {
 public:
  ScadaAddressSpaceBuilder(AddressSpaceImpl& address_space,
                           NodeFactory& node_factory);

  void BuildAll();

  ScadaAddressSpaceBuilder(const ScadaAddressSpaceBuilder&) = delete;
  ScadaAddressSpaceBuilder& operator=(const ScadaAddressSpaceBuilder&) = delete;

  AddressSpaceImpl& address_space_;
  NodeFactory& node_factory_;

 private:
  void CreateScadaAddressSpace();
  void CreateSecurityAddressSpace();
  void CreateDataItemAddressSpace();
  void CreateDeviceAddressSpace();
  void CreateHistoryAddressSpace();
  void CreateFileSystemAddressSpace();
  void CreateOpcAddressSpace();

  scada::Node* CreateNode(const scada::NodeState& node_state);

  scada::ObjectType* CreateObjectType(const scada::NodeId& id,
                                      scada::QualifiedName qualified_name,
                                      scada::LocalizedText display_name,
                                      const scada::NodeId& supertype_id);

  scada::VariableType* CreateVariableType(const scada::NodeId& id,
                                          scada::QualifiedName browse_name,
                                          scada::LocalizedText display_name,
                                          const scada::NodeId& data_type_id,
                                          const scada::NodeId& supertype_id);

  scada::DataType* CreateDataType(const scada::NodeId& id,
                                  scada::QualifiedName browse_name,
                                  scada::LocalizedText display_name,
                                  const scada::NodeId& supertype_id);

  scada::ReferenceType* CreateReferenceType(
      const scada::NodeId& reference_type_id,
      scada::QualifiedName browse_name,
      scada::LocalizedText display_name,
      const scada::NodeId& supertype_id);

  scada::ReferenceType* CreateReferenceType(
      const scada::NodeId& source_type_id,
      const scada::NodeId& reference_type_id,
      const scada::NodeId& category_id,
      scada::QualifiedName browse_name,
      scada::LocalizedText display_name,
      const scada::NodeId& target_type_id);

  void CreateEnumDataType(const scada::NodeId& datatype_id,
                          const scada::NodeId& enumstrings_id,
                          scada::QualifiedName browse_name,
                          scada::LocalizedText display_name,
                          std::vector<scada::LocalizedText> enum_strings);

  scada::ObjectType* CreateEventType(const scada::NodeId& event_type_id,
                                     const scada::QualifiedName& browse_name);

  void AddDataVariable(const scada::NodeId& type_id,
                       const scada::NodeId& variable_decl_id,
                       scada::QualifiedName browse_name,
                       scada::LocalizedText display_name,
                       const scada::NodeId& variable_type_id,
                       const scada::NodeId& data_type_id,
                       scada::Variant default_value = scada::Variant());

  scada::Variable* AddProperty(const scada::NodeId& type_id,
                               const scada::NodeId& prop_type_id,
                               const scada::NodeId& category_id,
                               scada::QualifiedName browse_name,
                               scada::LocalizedText display_name,
                               const scada::NodeId& data_type_id,
                               scada::Variant default_value);
};
