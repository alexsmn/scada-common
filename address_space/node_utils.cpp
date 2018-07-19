#include "address_space/node_utils.h"

#include "address_space/address_space.h"
#include "address_space/object.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "core/standard_node_ids.h"

namespace scada {

Object* AsObject(Node* node) {
  return node && node->GetNodeClass() == NodeClass::Object
             ? static_cast<Object*>(node)
             : nullptr;
}

const Object* AsObject(const Node* node) {
  return node && node->GetNodeClass() == NodeClass::Object
             ? static_cast<const Object*>(node)
             : nullptr;
}

const Variable* AsVariable(const Node* node) {
  return node && node->GetNodeClass() == NodeClass::Variable
             ? static_cast<const Variable*>(node)
             : nullptr;
}

Variable* AsVariable(Node* node) {
  return node && node->GetNodeClass() == NodeClass::Variable
             ? static_cast<Variable*>(node)
             : nullptr;
}

NodeId GetTypeDefinitionId(const Node& node) {
  auto* type_definition = node.type_definition();
  return type_definition ? type_definition->id() : NodeId{};
}

const ObjectType* AsObjectType(const Node* node) {
  return node && node->GetNodeClass() == NodeClass::ObjectType
             ? static_cast<const ObjectType*>(node)
             : nullptr;
}

ObjectType* AsObjectType(Node* node) {
  return node && node->GetNodeClass() == NodeClass::ObjectType
             ? static_cast<ObjectType*>(node)
             : nullptr;
}

const VariableType* AsVariableType(const Node* node) {
  return node && node->GetNodeClass() == NodeClass::VariableType
             ? static_cast<const VariableType*>(node)
             : nullptr;
}

VariableType* AsVariableType(Node* node) {
  return node && node->GetNodeClass() == NodeClass::VariableType
             ? static_cast<VariableType*>(node)
             : nullptr;
}

bool IsInstanceOf(const Node* node, const NodeId& type_id) {
  if (!node)
    return false;
  auto* type_definition = node->type_definition();
  return type_definition && IsSubtypeOf(*type_definition, type_id);
}

const ReferenceType* AsReferenceType(const Node* node) {
  return node && node->GetNodeClass() == NodeClass::ReferenceType
             ? static_cast<const ReferenceType*>(node)
             : nullptr;
}

const DataType* AsDataType(const Node* node) {
  return node && node->GetNodeClass() == NodeClass::DataType
             ? static_cast<const DataType*>(node)
             : nullptr;
}

const Node* GetReferenceTarget(const TypeDefinition* source,
                               const NodeId& reference_type_id) {
  for (; source; source = source->supertype()) {
    auto ref = GetReference(*source, reference_type_id);
    if (ref.node)
      return ref.node;
  }
  return nullptr;
}

const Node* GetAggregateDeclaration(const TypeDefinition& type,
                                    const NodeId& prop_decl_id) {
  for (auto* supertype = &type; supertype; supertype = supertype->supertype()) {
    for (auto* prop : GetAggregates(*supertype)) {
      if (prop->id() == prop_decl_id)
        return static_cast<const Variable*>(prop);
    }
  }
  return nullptr;
}

const Node* GetDeclaration(const Node& node) {
  assert(!IsTypeDefinition(node.GetNodeClass()));

  auto* parent = GetParent(node);
  if (!parent)
    return nullptr;

  const auto browse_name = node.GetBrowseName();
  for (auto* type = parent->type_definition(); type; type = type->supertype()) {
    if (auto* child = FindChild(*type, browse_name.name()))
      return child;
  }

  return nullptr;
}

const TypeDefinition* AsTypeDefinition(const Node* node) {
  return node && IsTypeDefinition(node->GetNodeClass())
             ? static_cast<const TypeDefinition*>(node)
             : nullptr;
}

TypeDefinition* AsTypeDefinition(Node* node) {
  return node && IsTypeDefinition(node->GetNodeClass())
             ? static_cast<TypeDefinition*>(node)
             : nullptr;
}

Reference GetParentReference(Node& node) {
  return FindReference(node, id::HierarchicalReferences, false);
}

Node* GetParent(Node& node) {
  return GetParentReference(node).node;
}

const Node* GetParent(const Node& node) {
  return GetParent(const_cast<Node&>(node));
}

Reference GetReference(const Node& source, const NodeId& reference_type_id) {
  return FindReference(source, reference_type_id, true);
}

Reference FindReference(const Node& node,
                        const ReferenceDescription& reference) {
  auto& refs =
      reference.forward ? node.forward_references() : node.inverse_references();
  for (auto& ref : refs) {
    if (IsSubtypeOf(*ref.type, reference.reference_type_id)) {
      if (ref.node->id() == reference.node_id)
        return ref;
    }
  }
  return {};
}

Reference FindReference(const Node& node,
                        const NodeId& reference_type_id,
                        bool forward,
                        const NodeId& referenced_node_id) {
  auto& refs = forward ? node.forward_references() : node.inverse_references();
  for (auto& ref : refs) {
    if (IsSubtypeOf(*ref.type, reference_type_id)) {
      if (ref.node->id() == referenced_node_id)
        return ref;
    }
  }
  return {};
}

Reference FindReference(const Node& node,
                        const NodeId& reference_type_id,
                        bool forward) {
  auto& refs = forward ? node.forward_references() : node.inverse_references();
  for (auto& ref : refs) {
    if (IsSubtypeOf(*ref.type, reference_type_id))
      return ref;
  }
  return {};
}

bool IsRefSubtypeOf::operator()(const Reference& ref) const {
  return IsSubtypeOf(*ref.type, ref_supertype_id_);
}

bool IsNonPropReference::operator()(const Reference& ref) const {
  return !IsSubtypeOf(*ref.type, id::NonHierarchicalReferences);
}

bool IsSubtypeOf(const TypeDefinition& type, const NodeId& supertype_id) {
  for (auto* supertype = &type; supertype; supertype = supertype->supertype()) {
    if (supertype->id() == supertype_id)
      return true;
  }
  return false;
}

bool IsSubtypeOf(AddressSpace& address_space,
                 const NodeId& type_id,
                 const NodeId& supertype_id) {
  const auto* type = AsTypeDefinition(address_space.GetNode(type_id));
  for (; type; type = type->supertype()) {
    if (type->id() == supertype_id)
      return true;
  }
  return false;
}

NodeId GetModellingRuleId(const Node& node) {
  assert(!scada::IsTypeDefinition(node.GetNodeClass()));
  auto* modelling_rule = GetReference(node, id::HasModellingRule).node;
  return modelling_rule ? modelling_rule->id() : NodeId();
}

Variant GetPropertyValue(const Node& node, const NodeId& prop_decl_id) {
  switch (node.GetNodeClass()) {
    case NodeClass::Object:
      return static_cast<const Object&>(node).GetPropertyValue(prop_decl_id);
    case NodeClass::Variable:
      return static_cast<const Variable&>(node).GetPropertyValue(prop_decl_id);
    default:
      return {};
  }
}

Status SetPropertyValue(Node& node,
                        const NodeId& prop_decl_id,
                        const Variant& value) {
  return node.SetPropertyValue(prop_decl_id, value);
}

void AddReference(scada::AddressSpace& address_space,
                  const scada::NodeId& reference_type_id,
                  const scada::NodeId& source_id,
                  const scada::NodeId& target_id) {
  auto* source = address_space.GetNode(source_id);
  assert(source);

  auto* target = address_space.GetNode(target_id);
  assert(target);

  AddReference(address_space, reference_type_id, *source, *target);
}

void AddReference(scada::AddressSpace& address_space,
                  const scada::NodeId& reference_type_id,
                  scada::Node& source,
                  scada::Node& target) {
  auto* reference_type =
      AsReferenceType(address_space.GetNode(reference_type_id));
  assert(reference_type);
  scada::AddReference(*reference_type, source, target);
}

void DeleteReference(scada::AddressSpace& address_space,
                     const scada::NodeId& reference_type_id,
                     const scada::NodeId& source_id,
                     const scada::NodeId& target_id) {
  auto* reference_type =
      AsReferenceType(address_space.GetNode(reference_type_id));
  assert(reference_type);

  auto* source = address_space.GetNode(source_id);
  assert(source);

  auto* target = address_space.GetNode(target_id);
  assert(target);

  scada::DeleteReference(*reference_type, *source, *target);
}

Node* FindChild(const Node& parent, base::StringPiece browse_name) {
  for (auto* child : GetChildren(parent)) {
    if (child->GetBrowseName().name() == browse_name)
      return child;
  }
  return nullptr;
}

Node* FindChildByDisplayName(const Node& parent,
                             base::StringPiece16 display_name) {
  for (auto* child : GetChildren(parent)) {
    if (child->GetDisplayName() == display_name)
      return child;
  }
  return nullptr;
}

Node* FindChildDeclaration(const TypeDefinition& type,
                           base::StringPiece browse_name) {
  for (auto* supertype = &type; supertype; supertype = supertype->supertype()) {
    if (auto* declaration = FindChild(*supertype, browse_name))
      return declaration;
  }
  return nullptr;
}

Node* FindChildComponent(const Node& parent, base::StringPiece browse_name) {
  for (auto* component : GetComponents(parent)) {
    if (component->GetBrowseName().name() == browse_name)
      return component;
  }
  return nullptr;
}

Node* FindComponentDeclaration(const TypeDefinition& type,
                               base::StringPiece browse_name) {
  for (auto* supertype = &type; supertype; supertype = supertype->supertype()) {
    if (auto* declaration = FindChildComponent(*supertype, browse_name))
      return declaration;
  }
  return nullptr;
}

void DeleteAllReferences(Node& node) {
  while (!node.inverse_references().empty()) {
    auto ref = node.inverse_references().back();
    DeleteReference(*ref.type, *ref.node, node);
  }
  while (!node.forward_references().empty()) {
    auto ref = node.forward_references().back();
    DeleteReference(*ref.type, node, *ref.node);
  }
}

}  // namespace scada
