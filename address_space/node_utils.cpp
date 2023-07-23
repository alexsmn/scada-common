#include "address_space/node_utils.h"

#include "address_space/address_space.h"
#include "address_space/object.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/range_util.h"
#include "scada/standard_node_ids.h"
#include "scada/view_service.h"

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

Object& AsObject(Node& node) {
  assert(node.GetNodeClass() == NodeClass::Object);
  return static_cast<Object&>(node);
}

const Object& AsObject(const Node& node) {
  assert(node.GetNodeClass() == NodeClass::Object);
  return static_cast<const Object&>(node);
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

Variable& AsVariable(Node& node) {
  assert(node.GetNodeClass() == NodeClass::Variable);
  return static_cast<Variable&>(node);
}

const Variable& AsVariable(const Node& node) {
  assert(node.GetNodeClass() == NodeClass::Variable);
  return static_cast<const Variable&>(node);
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

bool IsChildOf(const Node* node, const NodeId& parent_id) {
  while (node && node->id() != parent_id)
    node = GetParent(*node);
  return node != nullptr;
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

const DataType& AsDataType(const Node& node) {
  assert(node.GetNodeClass() == NodeClass::DataType);
  return static_cast<const DataType&>(node);
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

TypeDefinition* AsTypeDefinition(Node* node) {
  return node && IsTypeDefinition(node->GetNodeClass())
             ? static_cast<TypeDefinition*>(node)
             : nullptr;
}

const TypeDefinition* AsTypeDefinition(const Node* node) {
  return node && IsTypeDefinition(node->GetNodeClass())
             ? static_cast<const TypeDefinition*>(node)
             : nullptr;
}

TypeDefinition& AsTypeDefinition(Node& node) {
  assert(IsTypeDefinition(node.GetNodeClass()));
  return static_cast<TypeDefinition&>(node);
}

const TypeDefinition& AsTypeDefinition(const Node& node) {
  assert(IsTypeDefinition(node.GetNodeClass()));
  return static_cast<const TypeDefinition&>(node);
}

const TypeDefinition& AsTypeDefinition(const ReferenceType& reference_type) {
  return reference_type;
}

Reference GetParentReference(const Node& node) {
  return FindReference(node, id::HierarchicalReferences, false);
}

Node* GetParent(Node& node) {
  return GetParentReference(node).node;
}

const Node* GetParent(const Node& node) {
  return GetParent(const_cast<Node&>(node));
}

NodeId GetParentId(const Node& node) {
  auto* parent = GetParent(node);
  return parent ? parent->id() : NodeId{};
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
  if (include_subtypes_)
    return IsSubtypeOf(*ref.type, reference_type_id_);
  else
    return ref.type->id() == reference_type_id_;
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

NodeId GetModellingRuleId(const Node& node) {
  assert(!IsTypeDefinition(node.GetNodeClass()));
  auto* modelling_rule = GetReference(node, id::HasModellingRule).node;
  return modelling_rule ? modelling_rule->id() : NodeId();
}

Variant GetPropertyValue(const Node& node, const NodeId& prop_decl_id) {
  auto* declaration = FindDeclaration(node, prop_decl_id);
  assert(declaration);
  if (!declaration)
    return {};

  auto* property =
      AsVariable(FindChild(node, declaration->GetBrowseName().name()));
  if (!property)
    return {};

  return property->GetValue().value;
}

Status SetPropertyValue(Node& node,
                        const NodeId& prop_decl_id,
                        const Variant& value) {
  auto* declaration = FindDeclaration(node, prop_decl_id);
  assert(declaration);
  if (!declaration)
    return StatusCode::Bad;

  auto* property =
      AsVariable(FindChild(node, declaration->GetBrowseName().name()));
  assert(property);
  if (!property)
    return StatusCode::Bad;

  return property->SetValue({value, {}, {}, {}});
}

const Node* FindDeclaration(const Node& node, const NodeId& declaration_id) {
  auto* type = node.type_definition();
  if (!type)
    return nullptr;

  return GetPropertyDeclaration(*type, declaration_id);
}

const Variable* GetPropertyDeclaration(const TypeDefinition& type,
                                       const NodeId& prop_decl_id) {
  for (auto* supertype = &type; supertype; supertype = supertype->supertype()) {
    for (const auto& prop : GetProperties(*supertype)) {
      assert(prop.GetNodeClass() == NodeClass::Variable);
      if (prop.id() == prop_decl_id)
        return &prop;
    }
  }
  return nullptr;
}

Variant GetPropertyValueHelper(const Node& node, const NodeId& prop_decl_id) {
  auto* declaration = FindDeclaration(node, prop_decl_id);
  assert(declaration);
  if (!declaration)
    return {};

  const auto* property =
      AsVariable(FindChild(node, declaration->GetBrowseName().name()));
  assert(property);
  if (!property)
    return {};

  return property->GetValue().value;
}

Status SetPropertyValueHelper(Node& node,
                              const NodeId& prop_decl_id,
                              const Variant& value) {
  auto* declaration = FindDeclaration(node, prop_decl_id);
  assert(declaration);
  if (!declaration)
    return StatusCode::Bad_WrongPropertyId;

  auto* property =
      AsVariable(FindChild(node, declaration->GetBrowseName().name()));
  assert(property);
  if (!property)
    return StatusCode::Bad_WrongPropertyId;

  return property->SetValue({value, {}, {}, {}});
}

Node* FindChild(const Node& parent, std::string_view browse_name) {
  for (auto* child : GetChildren(parent)) {
    if (child->GetBrowseName().name() == browse_name)
      return child;
  }
  return nullptr;
}

Node* FindChildByDisplayName(const Node& parent,
                             std::u16string_view display_name) {
  for (auto* child : GetChildren(parent)) {
    if (child->GetDisplayName() == display_name)
      return child;
  }
  return nullptr;
}

Node* FindChildDeclaration(const TypeDefinition& type,
                           std::string_view browse_name) {
  for (auto* supertype = &type; supertype; supertype = supertype->supertype()) {
    if (auto* declaration = FindChild(*supertype, browse_name))
      return declaration;
  }
  return nullptr;
}

Node* FindChildComponent(const Node& parent, std::string_view browse_name) {
  for (auto* component : GetComponents(parent)) {
    if (component->GetBrowseName().name() == browse_name)
      return component;
  }
  return nullptr;
}

Node* FindComponentDeclaration(const TypeDefinition& type,
                               std::string_view browse_name) {
  for (auto* supertype = &type; supertype; supertype = supertype->supertype()) {
    if (auto* declaration = FindChildComponent(*supertype, browse_name))
      return declaration;
  }
  return nullptr;
}

NodeId GetNodeId(const Node* node) {
  return node ? node->id() : NodeId{};
}

NodeId GetSupertypeId(const Node& node) {
  const auto* type = AsTypeDefinition(&node);
  const auto* supertype = type ? type->supertype() : nullptr;
  return supertype ? supertype->id() : NodeId{};
}

const Node* FindComponentDeclaration(const Node& component) {
  auto* instance = GetParent(component);
  if (!instance) {
    return {};
  }

  auto* type = instance->type_definition();
  const auto& component_name = component.GetBrowseName();
  return FindComponentDeclaration(*type, component_name.name());
}

NodeState MakeNodeState(const Node& node) {
  auto properties =
      GetProperties(node) |
      boost::adaptors::transformed([](const Variable& prop) {
        return NodeProperty{GetNodeId(FindComponentDeclaration(prop)),
                            prop.GetValue().value};
      }) |
      to_vector;

  auto references =
      node.forward_references() |
      boost::adaptors::filtered([](const Reference& ref) {
        return IsSubtypeOf(*ref.type, id::NonHierarchicalReferences);
      }) |
      boost::adaptors::transformed([](const Reference& ref) {
        return ReferenceDescription{.reference_type_id = ref.type->id(),
                                    .forward = true,
                                    .node_id = ref.node->id()};
      }) |
      to_vector;

  auto parent_ref = GetParentReference(node);

  return {.node_id = node.id(),
          .node_class = node.GetNodeClass(),
          .type_definition_id = GetTypeDefinitionId(node),
          .parent_id = GetNodeId(parent_ref.node),
          .reference_type_id = GetNodeId(parent_ref.type),
          .attributes = {.browse_name = node.GetBrowseName(),
                         .display_name = node.GetDisplayName()},
          .properties = std::move(properties),
          .references = std::move(references),
          .supertype_id = GetSupertypeId(node)};
}

}  // namespace scada
