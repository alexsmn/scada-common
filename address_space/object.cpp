#include "address_space/object.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "core/standard_node_ids.h"
#include "address_space/type_definition.h"

namespace scada {

// GenericObject

Variant GenericObject::GetPropertyValue(const NodeId& prop_decl_id) const {
  auto i = properties_.find(prop_decl_id);
  return i == properties_.end() ? Variant() : i->second;
}

Status GenericObject::SetPropertyValue(const NodeId& prop_decl_id,
                                       const Variant& value) {
  if (value.is_null()) {
    properties_.erase(prop_decl_id);
    return StatusCode::Good;
  }

  properties_[prop_decl_id] = value;
  return StatusCode::Good;
}

// ComponentObject

bool ComponentObject::InitNode(AddressSpace& address_space,
                               const NodeId& instance_declaration_id) {
  instance_declaration_ =
      AsObject(address_space.GetNode(instance_declaration_id));
  if (!instance_declaration_)
    return false;

  auto* type = GetTypeDefinition(*instance_declaration_);
  if (!type)
    return false;

  scada::AddReference(address_space, id::HasTypeDefinition, *this, *type);
  return true;
}

}  // namespace scada
