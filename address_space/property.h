#pragma once

#include "address_space/address_space.h"
#include "address_space/node_utils.h"
#include "address_space/node_variable_handle.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"

namespace scada {

namespace detail {

template <class ValueType>
inline bool Convert(const Variant& value, ValueType& result) {
  return value.get(result);
}

template <>
inline bool Convert(const Variant& value, Variant& result) {
  result = value;
  return true;
}

}  // namespace detail

template <class ValueType>
class Property : public Variable {
 public:
  Property(AddressSpace& address_space,
           Node& parent,
           const NodeId& instance_declaration_id);

  const ValueType& value() const { return value_; }

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
  virtual DataValue GetValue() const override {
    auto now = DateTime::Now();
    return {value_, {}, now, now};
  }
  virtual Status SetValue(const DataValue& data_value) override;
  virtual DateTime GetChangeTime() const override { return {}; }

 private:
  Variable* instance_declaration_ = nullptr;
  ValueType value_{};
};

template <class ValueType>
inline Property<ValueType>::Property(AddressSpace& address_space,
                                     Node& parent,
                                     const NodeId& instance_declaration_id) {
  instance_declaration_ =
      AsVariable(address_space.GetNode(instance_declaration_id));
  if (!instance_declaration_)
    throw std::runtime_error("Instance declaration wasn't found");

  assert(!FindChild(parent, instance_declaration_->GetBrowseName().name()));

  auto* type = instance_declaration_->type_definition();
  if (!type)
    throw std::runtime_error("Instance declaration has no type definition");

  auto default_value = instance_declaration_->GetValue();
  if (!detail::Convert(default_value.value, value_))
    throw std::runtime_error("Wrong data type of default value");

  scada::AddReference(address_space, id::HasTypeDefinition, *this, *type);
  scada::AddReference(address_space, scada::id::HasProperty, parent, *this);
}

template <class ValueType>
inline Status Property<ValueType>::SetValue(const DataValue& data_value) {
  ValueType new_value{};
  // TODO: Wrong data type.
  if (!detail::Convert<ValueType>(data_value.value, new_value))
    return scada::StatusCode::Bad;

  if (value_ == new_value)
    return StatusCode::Good;

  value_ = std::move(new_value);

  if (auto variable_handle = variable_handle_.lock())
    variable_handle->ForwardData(data_value);

  return StatusCode::Good;
}

}  // namespace scada
