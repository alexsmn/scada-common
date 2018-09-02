#include "node.h"

#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "base/strings/sys_string_conversions.h"
#include "common/node_id_util.h"

namespace scada {

void AddReference(const ReferenceType& type, Node& source, Node& target) {
  source.AddReference(type, true, target);
  target.AddReference(type, false, source);
}

void DeleteReference(const ReferenceType& type, Node& source, Node& target) {
  source.DeleteReference(type, true, target);
  target.DeleteReference(type, false, source);
}

// Node

Node::Node() {}

Node::~Node() {
  assert(forward_references_.empty());
  assert(inverse_references_.empty());
}

void Node::AddReference(const ReferenceType& reference_type,
                        bool forward,
                        Node& node) {
  Reference ref{&reference_type, &node};
  auto& refs = forward ? forward_references_ : inverse_references_;
  assert(std::find(refs.begin(), refs.end(), ref) == refs.end());
  refs.emplace_back(ref);

  if (forward && reference_type.id() == scada::id::HasTypeDefinition) {
    assert(!type_definition_);
    assert(scada::AsTypeDefinition(&node));
    type_definition_ = scada::AsTypeDefinition(&node);
  }
}

void Node::DeleteReference(const ReferenceType& reference_type,
                           bool forward,
                           Node& node) {
  if (forward && reference_type.id() == scada::id::HasTypeDefinition) {
    assert(type_definition_ == &node);
    type_definition_ = nullptr;
  }

  Reference ref{&reference_type, &node};
  auto& refs = forward ? forward_references_ : inverse_references_;
  auto i = std::find(refs.rbegin(), refs.rend(), ref);
  assert(i != refs.rend());
  refs.erase(--i.base());
}

Variant Node::GetPropertyValue(const NodeId& prop_decl_id) const {
  return GetPropertyValueHelper(*this, prop_decl_id);
}

Status Node::SetPropertyValue(const NodeId& prop_decl_id,
                              const Variant& value) {
  return SetPropertyValueHelper(*this, prop_decl_id, value);
}

QualifiedName Node::GetBrowseName() const {
  if (!browse_name_.empty())
    return browse_name_;
  assert(!id_.is_null());
  return NodeIdToScadaString(id_);
}

LocalizedText Node::GetDisplayName() const {
  if (!display_name_.empty())
    return display_name_;
  return ToLocalizedText(GetBrowseName().name());
}

}  // namespace scada
