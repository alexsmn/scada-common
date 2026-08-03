#include "node.h"

#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "base/check.h"
#include "model/node_id_util.h"
#include "scada/authorization.h"

namespace scada {

// Node

Node::Node() {}

Node::~Node() {
  base::Check(forward_references_.empty());
  base::Check(inverse_references_.empty());
}

const std::vector<RolePermissionType>* Node::role_permissions() const {
  return role_permissions_.get();
}

void Node::SetRolePermissions(
    std::vector<RolePermissionType> role_permissions) {
  role_permissions_ =
      std::make_unique<std::vector<RolePermissionType>>(std::move(role_permissions));
}

void Node::AddReference(const ReferenceType& reference_type,
                        bool forward,
                        Node& node) {
  Reference ref{&reference_type, &node};
  auto& refs = forward ? forward_references_ : inverse_references_;
  base::Check(std::find(refs.begin(), refs.end(), ref) == refs.end());
  refs.emplace_back(ref);

  if (forward && reference_type.id() == scada::id::HasTypeDefinition) {
    base::Check(!type_definition_);
    base::Check(scada::AsTypeDefinition(&node));
    type_definition_ = scada::AsTypeDefinition(&node);
  }
}

void Node::DeleteReference(const ReferenceType& reference_type,
                           bool forward,
                           Node& node) {
  if (forward && reference_type.id() == scada::id::HasTypeDefinition) {
    base::Check(type_definition_ == &node);
    type_definition_ = nullptr;
  }

  Reference ref{&reference_type, &node};
  auto& refs = forward ? forward_references_ : inverse_references_;
  auto i = std::find(refs.rbegin(), refs.rend(), ref);
  base::Check(i != refs.rend());
  refs.erase(--i.base());
}

QualifiedName Node::GetBrowseName() const {
  if (!browse_name_.empty())
    return browse_name_;
  base::Check(!id_.is_null());
  return NodeIdToScadaString(id_);
}

LocalizedText Node::GetDisplayName() const {
  if (!display_name_.empty())
    return display_name_;
  return ToLocalizedText(GetBrowseName().name());
}

}  // namespace scada
