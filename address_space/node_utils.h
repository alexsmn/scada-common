#pragma once

#include "address_space/node.h"
#include "address_space/reference.h"
#include "base/ranges.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"

#include <string>

namespace scada {

class AddressSpace;
class DataType;
class Object;
class Node;
class NodeId;
class ObjectType;
class ReferenceType;
class TypeDefinition;
class Variable;
class VariableType;
struct ReferenceDescription;

Object* AsObject(Node* node);
const Object* AsObject(const Node* node);
Object& AsObject(Node& node);
const Object& AsObject(const Node& node);
const Variable* AsVariable(const Node* node);
Variable* AsVariable(Node* node);
const Variable& AsVariable(const Node& node);
Variable& AsVariable(Node& node);
const ObjectType* AsObjectType(const Node* node);
ObjectType* AsObjectType(Node* node);
const VariableType* AsVariableType(const Node* node);
VariableType* AsVariableType(Node* node);
const ReferenceType* AsReferenceType(const Node* node);
const DataType* AsDataType(const Node* node);
const DataType& AsDataType(const Node& node);
TypeDefinition* AsTypeDefinition(Node* node);
const TypeDefinition* AsTypeDefinition(const Node* node);
TypeDefinition& AsTypeDefinition(Node& node);
const TypeDefinition& AsTypeDefinition(const Node& node);

NodeId GetTypeDefinitionId(const Node& node);

const Node* GetReferenceTarget(const TypeDefinition* source,
                               const NodeId& reference_type_id);

const Node* GetAggregateDeclaration(const TypeDefinition& type,
                                    const NodeId& prop_decl_id);
const Node* GetDeclaration(const Node& node);

bool IsInstanceOf(const Node* node, const NodeId& type_id);

Reference GetParentReference(const Node& node);
Node* GetParent(Node& node);
const Node* GetParent(const Node& node);

Reference GetReference(const Node& source, const NodeId& reference_type_id);
Reference FindReference(const Node& node,
                        const NodeId& reference_type_id,
                        bool forward);
Reference FindReference(const Node& node,
                        const NodeId& reference_type_id,
                        bool forward,
                        const NodeId& referenced_node_id);
Reference FindReference(const Node& node,
                        const ReferenceDescription& reference);

inline Node* GetTarget(const Node& node,
                       const NodeId& reference_type_id,
                       bool forward) {
  return FindReference(node, reference_type_id, forward).node;
}

bool IsSubtypeOf(const TypeDefinition& type, const NodeId& supertype_id);
bool IsSubtypeOf(AddressSpace& address_space,
                 const NodeId& type_id,
                 const NodeId& supertype_id);

struct IsNonPropReference {
  bool operator()(const Reference& ref) const;
};

struct IsRefSubtypeOf {
  bool operator()(const Reference& ref) const;

  const NodeId reference_type_id_;
  bool include_subtypes_ = true;
};

inline auto GetNonPropReferences(const Node& node) {
  return MakeFilterRange(IsNonPropReference(), node.forward_references());
}

struct RefNode {
  Node* operator()(const Reference& ref) const { return ref.node; }
};

template <class References>
inline auto FilterReferences(const References& references,
                             const NodeId& reference_type_id,
                             bool include_subtypes = true) {
  return MakeFilterRange(IsRefSubtypeOf{reference_type_id, include_subtypes},
                         references);
}

template <class References>
inline auto GetNodes(const References& references) {
  return MakeTransformRange(RefNode(), references);
}

inline auto GetForwardReferenceNodes(const Node& node,
                                     const NodeId& reference_type_id) {
  return GetNodes(
      FilterReferences(node.forward_references(), reference_type_id));
}

inline auto GetProperties(const Node& node) {
  return GetForwardReferenceNodes(node, id::HasProperty);
}

inline auto GetComponents(const Node& node) {
  return GetForwardReferenceNodes(node, id::HasComponent);
}

inline auto GetAggregates(const Node& node) {
  return GetForwardReferenceNodes(node, id::Aggregates);
}

inline auto GetOrganizes(const Node& node) {
  return GetForwardReferenceNodes(node, id::Organizes);
}

inline auto GetChildren(const Node& node) {
  return GetForwardReferenceNodes(node, id::HierarchicalReferences);
}

NodeId GetModellingRuleId(const Node& node);

const Node* FindDeclaration(const Node& node, const NodeId& declaration_id);
const Variable* GetPropertyDeclaration(const TypeDefinition& type,
                                       const NodeId& prop_decl_id);

Variant GetPropertyValue(const Node& node, const NodeId& prop_decl_id);
Status SetPropertyValue(Node& node,
                        const NodeId& prop_decl_id,
                        const Variant& value);

Variant GetPropertyValueHelper(const Node& node, const NodeId& prop_decl_id);
Status SetPropertyValueHelper(Node& node,
                              const NodeId& prop_decl_id,
                              const Variant& value);

void AddReference(AddressSpace& address_space,
                  const NodeId& reference_type_id,
                  const NodeId& source_id,
                  const NodeId& target_id);
void AddReference(AddressSpace& address_space,
                  const NodeId& reference_type_id,
                  Node& source,
                  Node& target);
void DeleteReference(AddressSpace& address_space,
                     const NodeId& reference_type_id,
                     const NodeId& source_id,
                     const NodeId& target_id);
void DeleteAllReferences(Node& node);

Node* FindChild(const Node& parent, base::StringPiece browse_name);
Node* FindChildDeclaration(const TypeDefinition& type,
                           base::StringPiece browse_name);
Node* FindChildComponent(const Node& parent, base::StringPiece browse_name);
Node* FindComponentDeclaration(const TypeDefinition& type,
                               base::StringPiece browse_name);

Node* FindChildByDisplayName(const Node& parent,
                             base::StringPiece16 display_name);

}  // namespace scada
