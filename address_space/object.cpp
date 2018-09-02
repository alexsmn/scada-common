#include "address_space/object.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node_builder.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "core/standard_node_ids.h"

namespace scada {

// Object

void Object::Call(const scada::NodeId& method_id,
                  const std::vector<scada::Variant>& arguments,
                  const scada::NodeId& user_id,
                  const scada::StatusCallback& callback) {
  if (callback)
    callback(scada::StatusCode::Bad_WrongMethodId);
}

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

ComponentObject::ComponentObject(NodeBuilder& builder,
                                 const NodeId& instance_declaration_id)
    : instance_declaration_{
          AsObject(builder.GetInstanceDeclaration(instance_declaration_id))} {
  auto* type = instance_declaration_.type_definition();
  if (!type)
    throw std::runtime_error("Instance delaration has no type definition");

  builder.AddReference(id::HasTypeDefinition, *this, *type);
}

}  // namespace scada
