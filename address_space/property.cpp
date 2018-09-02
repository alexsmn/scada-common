#include "address_space/property.h"

#include "address_space/address_space.h"
#include "address_space/node_utils.h"
#include "address_space/node_variable_handle.h"
#include "address_space/type_definition.h"

namespace scada {

// Property

Property::Property(AddressSpace& address_space,
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

  value_ = instance_declaration_->GetValue();

  scada::AddReference(address_space, id::HasTypeDefinition, *this, *type);
  scada::AddReference(address_space, scada::id::HasProperty, parent, *this);
}

Status Property::SetValue(const DataValue& data_value) {
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
