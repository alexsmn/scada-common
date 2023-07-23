#include "address_space/object.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node_builder.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "scada/standard_node_ids.h"

namespace scada {

// ComponentObject

ComponentObject::ComponentObject(NodeBuilder& builder,
                                 const NodeId& instance_declaration_id)
    : instance_declaration_{
          AsObject(builder.GetMutableNode(instance_declaration_id))} {
  auto* type = instance_declaration_.type_definition();
  if (!type)
    throw std::runtime_error("Instance delaration has no type definition");

  builder.AddReference(id::HasTypeDefinition, *this, *type);
}

}  // namespace scada
