#pragma once

#include "address_space/address_space.h"
#include "address_space/node.h"
#include "address_space/node_utils.h"
#include "address_space/node_variable_handle.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/time/time.h"
#include "core/configuration_types.h"
#include "core/data_value.h"
#include "core/standard_node_ids.h"

namespace scada {

template <class ValueType>
class DataVariable : public Variable {
 public:
  DataVariable(AddressSpace& address_space,
               Node& parent,
               const NodeId& instance_declaration_id);

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
  virtual DateTime GetChangeTime() const override { return change_time_; }

 private:
  Variable* instance_declaration_ = nullptr;
  DataValue value_;
  DateTime change_time_;
};

// DataVariable

template <class ValueType>
inline DataVariable<ValueType>::DataVariable(
    AddressSpace& address_space,
    Node& parent,
    const NodeId& instance_declaration_id) {
  instance_declaration_ =
      AsVariable(address_space.GetNode(instance_declaration_id));
  if (!instance_declaration_)
    throw std::runtime_error("Instance declaration wasn't found");

  auto* type = instance_declaration_->type_definition();
  if (!type)
    throw std::runtime_error("Instance declaration has no type definition");

  value_ = instance_declaration_->GetValue();

  scada::AddReference(address_space, id::HasTypeDefinition, *this, *type);
  scada::AddReference(address_space, scada::id::HasComponent, parent, *this);
}

template <class ValueType>
inline Status DataVariable<ValueType>::SetValue(const DataValue& data_value) {
  if (value_ == data_value)
    return StatusCode::Good;

  bool is_current = IsUpdate(value_, data_value);
  if (is_current)
    value_ = data_value;

  if (auto variable_handle = variable_handle_.lock())
    variable_handle->ForwardData(data_value);

  return StatusCode::Good;
}

}  // namespace scada
