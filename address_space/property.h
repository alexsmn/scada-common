#pragma once

#include "address_space/variable.h"

namespace scada {

class Property : public Variable {
 public:
  Property(AddressSpace& address_space,
           Node& parent,
           const NodeId& instance_declaration_id);

  const scada::Variant& value() const { return value_.value; }

  // Node
  virtual QualifiedName GetBrowseName() const override {
    return instance_declaration_->GetBrowseName();
  }
  virtual LocalizedText GetDisplayName() const override {
    return instance_declaration_->GetDisplayName();
  }

  // Variable
  virtual const DataType& GetDataType() const override {
    return instance_declaration_->GetDataType();
  }
  virtual DataValue GetValue() const override { return value_; }
  virtual Status SetValue(const DataValue& data_value) override;
  virtual DateTime GetChangeTime() const override { return {}; }

 private:
  Variable* instance_declaration_ = nullptr;
  DataValue value_;
};

} // namespace scada
