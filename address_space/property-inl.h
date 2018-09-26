#pragma once

#include "address_space/address_space.h"
#include "address_space/node_builder.h"
#include "address_space/node_utils.h"
#include "address_space/node_variable_handle.h"
#include "address_space/property.h"
#include "address_space/type_definition.h"

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

// PropertyDecl

template <class ValueType>
inline PropertyDecl<ValueType>::PropertyDecl(NodeBuilder& builder,
                                             TypeDefinition& parent,
                                             NodeId node_id,
                                             QualifiedName browse_name,
                                             LocalizedText display_name,
                                             const NodeId& data_type_id)
    : data_type_{scada::AsDataType(builder.GetNode(data_type_id))} {
  assert(!FindChild(parent, browse_name.name()));

  set_id(std::move(node_id));
  SetBrowseName(std::move(browse_name));
  SetDisplayName(std::move(display_name));

  auto& type_definition =
      scada::AsTypeDefinition(builder.GetNode(scada::id::PropertyType));

  builder.AddReference(id::HasTypeDefinition, *this, type_definition);
  builder.AddReference(scada::id::HasProperty, parent, *this);
}

// Property

template <class ValueType>
inline Property<ValueType>::Property(NodeBuilder& builder,
                                     Node& parent,
                                     const NodeId& instance_declaration_id)
    : instance_declaration_{
          AsVariable(builder.GetNode(instance_declaration_id))} {
  assert(!FindChild(parent, instance_declaration_.GetBrowseName().name()));

  auto* type_definition = instance_declaration_.type_definition();
  assert(type_definition);
  if (!type_definition)
    throw std::runtime_error("Instance declaration has no type definition");

  auto default_value = instance_declaration_.GetValue();
  if (!detail::Convert(default_value.value, value_))
    throw std::runtime_error("Wrong data type of default value");

  builder.AddReference(id::HasTypeDefinition, *this, *type_definition);
  builder.AddReference(scada::id::HasProperty, parent, *this);
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

  if (auto variable_handle = variable_handle_.lock()) {
    auto now = scada::DateTime::Now();
    variable_handle->ForwardData(scada::DataValue{value_, {}, now, now});
  }

  return StatusCode::Good;
}

}  // namespace scada
