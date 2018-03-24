#include "core/type_definition.h"

#include "core/node_utils.h"
#include "core/reference.h"
#include "core/standard_node_ids.h"

namespace scada {

TypeDefinition::TypeDefinition() {
}

Variant DataType::GetPropertyValue(const NodeId& prop_decl_id) const {
  if (prop_decl_id == id::EnumStrings)
    return enum_strings;

  return TypeDefinition::GetPropertyValue(prop_decl_id);
}

} // namespace scada