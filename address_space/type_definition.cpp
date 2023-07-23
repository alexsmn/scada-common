#include "address_space/type_definition.h"

#include "address_space/node_utils.h"
#include "address_space/reference.h"
#include "scada/standard_node_ids.h"

namespace scada {

// TypeDefinition

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

}  // namespace scada
