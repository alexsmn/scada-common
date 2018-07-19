#include "address_space/type_definition.h"

#include "address_space/node_utils.h"
#include "address_space/reference.h"
#include "core/standard_node_ids.h"

namespace scada {

TypeDefinition::TypeDefinition() {}

void TypeDefinition::AddReference(const ReferenceType& reference_type,
                                  bool forward,
                                  Node& node) {
  Node::AddReference(reference_type, forward, node);

  if (!forward && reference_type.id() == scada::id::HasSubtype) {
    assert(!supertype_);
    assert(scada::AsTypeDefinition(&node));
    supertype_ = scada::AsTypeDefinition(&node);
  }
}

void TypeDefinition::DeleteReference(const ReferenceType& reference_type,
                                     bool forward,
                                     Node& node) {
  if (!forward && reference_type.id() == scada::id::HasSubtype) {
    assert(supertype_ == &node);
    supertype_ = nullptr;
  }

  Node::DeleteReference(reference_type, forward, node);
}

Variant DataType::GetPropertyValue(const NodeId& prop_decl_id) const {
  if (prop_decl_id == id::EnumStrings)
    return enum_strings;

  return TypeDefinition::GetPropertyValue(prop_decl_id);
}

Status DataType::SetPropertyValue(const NodeId& prop_decl_id,
                                  const Variant& value) {
  if (prop_decl_id == id::EnumStrings) {
    auto* v = value.get_if<decltype(enum_strings)>();
    if (!v)
      return scada::StatusCode::Bad_WrongTypeId;
    enum_strings = *v;
    return scada::StatusCode::Good;
  }

  return TypeDefinition::SetPropertyValue(prop_decl_id, value);
}

}  // namespace scada
