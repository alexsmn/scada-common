#pragma once

#include "core/node.h"
#include "core/variant.h"

namespace scada {

class TypeDefinition : public Node {
 public:
  TypeDefinition();
};

class DataType : public TypeDefinition {
 public:
  DataType(NodeId id, QualifiedName browse_name, LocalizedText display_name) {
    set_id(std::move(id));
    SetBrowseName(std::move(browse_name));
    SetDisplayName(std::move(display_name));
  }

  // TypeDefinition
  virtual NodeClass GetNodeClass() const override { return NodeClass::DataType; }
  virtual Variant GetPropertyValue(const NodeId& prop_decl_id) const override;

  std::vector<scada::LocalizedText> enum_strings;
};

class VariableType : public TypeDefinition {
 public:
  VariableType(const NodeId& id, QualifiedName browse_name, LocalizedText display_name, const DataType& data_type)
      : data_type_(data_type) {
    set_id(id);
    SetBrowseName(std::move(browse_name));
    SetDisplayName(std::move(display_name));
  }

  const DataType& data_type() const { return data_type_; }

  const Variant& default_value() const { return default_value_; }
  void set_default_value(scada::Variant value) { default_value_ = std::move(value); }

  // TypeDefinition
  virtual NodeClass GetNodeClass() const override { return NodeClass::VariableType; }

 private:
  const DataType& data_type_;
  Variant default_value_;
};

class ReferenceType : public TypeDefinition {
 public:
  ReferenceType(const NodeId& id, QualifiedName browse_name, LocalizedText display_name) {
    set_id(id);
    SetBrowseName(std::move(browse_name));
    SetDisplayName(std::move(display_name));
  }

  // TypeDefinition
  virtual NodeClass GetNodeClass() const override { return NodeClass::ReferenceType; }
};

class ObjectType : public TypeDefinition {
 public:
  ObjectType(const NodeId& id, QualifiedName browse_name, LocalizedText display_name) {
    set_id(id);
    SetBrowseName(std::move(browse_name));
    SetDisplayName(std::move(display_name));
  }

  // TypeDefinition
  virtual NodeClass GetNodeClass() const override { return NodeClass::ObjectType; }
};

} // namespace scada