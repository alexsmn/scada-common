#pragma once

#include "core/standard_node_ids.h"
#include "core/view_service.h"

namespace test {

template<class T>
inline scada::DataValue MakeReadResult(T&& value) {
  auto timestamp = scada::DateTime::Now();
  return {std::forward<T>(value), {}, timestamp, timestamp};
}

inline scada::DataValue Read(const scada::BrowseNode& node, scada::AttributeId attribute_id) {
  switch (attribute_id) {
    case scada::AttributeId::NodeId:
      return MakeReadResult(node.node_id);
    case scada::AttributeId::NodeClass:
      return MakeReadResult(static_cast<int32_t>(node.node_class));
    case scada::AttributeId::BrowseName:
      return MakeReadResult(node.browse_name);
    case scada::AttributeId::DisplayName:
      return MakeReadResult(node.display_name);
    default:
      return {scada::StatusCode::Bad, scada::DateTime::Now()};
  }
}

class TestAddressSpace : public scada::ViewService {
 public:
  const scada::BrowseNode* GetNode(const scada::NodeId& node_id) const;

  scada::DataValue Read(const scada::ReadValueId& read_id) const;

  scada::BrowseResult Browse(const scada::BrowseDescription& description) const;

  scada::NodeId GetSupertypeId(const scada::NodeId& type_id) const;
  bool IsSubtypeOf(const scada::NodeId& type_id, const scada::NodeId& supertype_id) const;

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& nodes, const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePath(const scada::NodeId& starting_node_id, const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback);
  virtual void Subscribe(scada::ViewEvents& events) override {}
  virtual void Unsubscribe(scada::ViewEvents& events) override {}

  const scada::NodeId kHasTypeDefinitionRefTypeId{"kHasTypeDefinitionRefTypeId", 0};
  const scada::NodeId kHasParentRefTypeId{"HasParentRefTypeId", 0};

  const scada::NodeId kTestTypeId{"TestTypeId", 0};
  const scada::NodeId kTestProp1Id{"TestProp1Id", 0};
  const scada::NodeId kTestProp2Id{"TestProp2Id", 0};
  const scada::NodeId kTestRefTypeId{"TestRefTypeId", 0};

  const scada::BrowseNode kRootNode{
      {}, // parent id
      {}, // reference_type_id
      scada::id::RootFolder,
      scada::NodeClass::Object,
      scada::id::FolderType, // type id
      "Root",
      LOCALIZED_TEXT("Root Display Name"),
  };

  const scada::BrowseNode kTestNode1{
      kRootNode.node_id, // parent id
      kHasParentRefTypeId, // reference type id
      scada::NodeId{"TestNode1Id", 0},
      scada::NodeClass::Object,
      kTestTypeId, // type id
      "TestNode1",
      LOCALIZED_TEXT("TestNode1 Display Name"),
  };

  const scada::BrowseNode kTestNode2{
      kRootNode.node_id, // parent id
      kHasParentRefTypeId, // reference type id
      scada::NodeId{"TestNode2Id", 0},
      scada::NodeClass::Object,
      kTestTypeId, // type id
      "TestNode2",
      LOCALIZED_TEXT("TestNode2 Display Name"),
  };

  const std::vector<scada::BrowseNode> kNodes{
      kRootNode,
      kTestNode1,
      kTestNode2,
  };

  const std::vector<scada::ViewReference> kReferences{
      {kTestRefTypeId, kTestNode1.node_id, kTestNode2.node_id},
  };

 private:
  bool Matches(const scada::ReferenceDescription& reference, const scada::BrowseDescription& description) const;
  void ReturnIfMatches(std::vector<scada::ReferenceDescription>& references, const scada::BrowseDescription& description,
      scada::ReferenceDescription reference) const;
};

inline const scada::BrowseNode* TestAddressSpace::GetNode(const scada::NodeId& node_id) const {
  for (auto& node : kNodes) {
    if (node.node_id == node_id)
      return &node;
  }
  return nullptr;
}

inline scada::DataValue TestAddressSpace::Read(const scada::ReadValueId& read_id) const {
  auto* node = GetNode(read_id.node_id);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};

  return test::Read(*node, read_id.attribute_id);
}

inline scada::BrowseResult TestAddressSpace::Browse(const scada::BrowseDescription& description) const {
  auto* node = GetNode(description.node_id);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId};

  std::vector<scada::ReferenceDescription> references;

  if (!node->type_id.is_null())
    ReturnIfMatches(references, description, {scada::id::HasTypeDefinition, false, node->type_id});
  if (!node->reference_type_id.is_null())
    ReturnIfMatches(references, description, {node->reference_type_id, false, node->parent_id});

  for (auto& child_node : kNodes) {
    if (child_node.parent_id == node->node_id)
      ReturnIfMatches(references, description, {child_node.reference_type_id, true, child_node.node_id});
  }

  for (auto& ref : kReferences) {
    if (ref.source_id != node->node_id && ref.target_id != node->node_id)
      continue;
    bool forward = ref.source_id == node->node_id;
    auto& node_id = forward ? ref.target_id : ref.source_id;
    ReturnIfMatches(references, description, {ref.reference_type_id, forward, node_id});
  }

  return {scada::StatusCode::Good, std::move(references)};
}

inline scada::NodeId TestAddressSpace::GetSupertypeId(const scada::NodeId& type_id) const {
  auto* node = GetNode(type_id);
  if (!node)
    return {};
  if (node->reference_type_id != scada::id::HasSubtype)
    return {};
  return node->parent_id;
}

inline bool TestAddressSpace::IsSubtypeOf(const scada::NodeId& type_id, const scada::NodeId& supertype_id) const {
  for (auto node_id = type_id; !node_id.is_null(); node_id = GetSupertypeId(node_id)) {
    if (node_id == supertype_id)
      return true;
  }
  return false;
}

inline bool TestAddressSpace::Matches(const scada::ReferenceDescription& reference, const scada::BrowseDescription& description) const {
  if (description.direction != scada::BrowseDirection::Both) {
    bool forward = description.direction == scada::BrowseDirection::Forward;
    return reference.forward == forward;
  }

  bool reference_type_matches = description.include_subtypes ?
      IsSubtypeOf(reference.reference_type_id, description.reference_type_id) :
      reference.reference_type_id == description.reference_type_id;
  if (!reference_type_matches)
    return false;

  return true;
}

inline void TestAddressSpace::ReturnIfMatches(std::vector<scada::ReferenceDescription>& references,
    const scada::BrowseDescription& description, scada::ReferenceDescription reference) const {
  if (Matches(reference, description))
    references.emplace_back(std::move(reference));
}

inline void TestAddressSpace::Browse(const std::vector<scada::BrowseDescription>& nodes, const scada::BrowseCallback& callback) {
  std::vector<scada::BrowseResult> results(nodes.size());
  for (size_t i = 0; i < nodes.size(); ++i)
    results[i] = Browse(nodes[i]);
  callback(scada::StatusCode::Good, std::move(results));
}

void TestAddressSpace::TranslateBrowsePath(const scada::NodeId& starting_node_id, const scada::RelativePath& relative_path,
    const scada::TranslateBrowsePathCallback& callback) {
  callback(scada::StatusCode::Bad, {}, 0);
}

} // namespace test
