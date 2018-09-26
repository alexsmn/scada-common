#pragma once

#include "address_space/variable.h"

namespace scada {

class NodeBuilder;

template <class ValueType>
class PropertyDecl : public Variable {
 public:
  PropertyDecl(NodeBuilder& builder,
               TypeDefinition& parent,
               NodeId node_id,
               QualifiedName browse_name,
               LocalizedText display_name,
               const NodeId& data_type_id);

  const ValueType& value() const { return value_; }
  void set_value(ValueType value) { value_ = std::move(value); }

  // Variable
  virtual const DataType& GetDataType() const override { return data_type_; }
  virtual DataValue GetValue() const override {
    auto now = DateTime::Now();
    return {value_, {}, now, now};
  }
  virtual Status SetValue(const DataValue& data_value) override {
    return scada::StatusCode::Bad;
  }
  virtual DateTime GetChangeTime() const override { return {}; }

 private:
  const scada::DataType& data_type_;
  ValueType value_{};
};

template <class ValueType>
class Property : public Variable {
 public:
  Property(NodeBuilder& builder,
           Node& parent,
           const NodeId& instance_declaration_id);

  const ValueType& value() const { return value_; }
  void set_value(ValueType value) { value_ = std::move(value); }

  // Node
  virtual QualifiedName GetBrowseName() const override {
    return instance_declaration_.GetBrowseName();
  }
  virtual LocalizedText GetDisplayName() const override {
    return instance_declaration_.GetDisplayName();
  }

  // Variable
  virtual const DataType& GetDataType() const override {
    return instance_declaration_.GetDataType();
  }
  virtual DataValue GetValue() const override {
    auto now = DateTime::Now();
    return {value_, {}, now, now};
  }
  virtual Status SetValue(const DataValue& data_value) override;
  virtual DateTime GetChangeTime() const override { return {}; }

 private:
  Variable& instance_declaration_;
  ValueType value_{};
};

}  // namespace scada

#include "address_space/property-inl.h"
