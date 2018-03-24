#pragma once

#include "base/observer_list.h"
#include "common/node_state.h"
#include "core/attribute_service.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/view_service.h"

namespace test {

class TestAddressSpace : public scada::AttributeService,
                         public scada::ViewService {
 public:
  scada::NodeState* GetNode(const scada::NodeId& node_id);
  const scada::NodeState* GetNode(const scada::NodeId& node_id) const;

  scada::DataValue Read(const scada::ReadValueId& read_id) const;
  scada::BrowseResult Browse(const scada::BrowseDescription& description) const;

  void DeleteNode(const scada::NodeId& node_id);

  void NotifyModelChanged(const scada::ModelChangeEvent& event);
  void NotifyNodeSemanticsChanged(const scada::NodeId& node_id);

  // scada::AttributeService
  virtual void Read(const std::vector<scada::ReadValueId>& value_ids,
                    const scada::ReadCallback& callback) override;
  virtual void Write(const scada::NodeId& node_id,
                     double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const scada::StatusCallback& callback) override;

  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& nodes,
                      const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePath(
      const scada::NodeId& starting_node_id,
      const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback) override;
  virtual void Subscribe(scada::ViewEvents& events) override {
    view_events_.AddObserver(&events);
  }
  virtual void Unsubscribe(scada::ViewEvents& events) override {
    view_events_.RemoveObserver(&events);
  }

  static scada::NodeId MakeNestedNodeId(const scada::NodeId& parent_id,
                                        const scada::NodeId& child_id);
  static bool IsNestedNodeId(const scada::NodeId& node_id,
                             scada::NodeId& parent_id,
                             scada::NodeId& child_id);

  static const unsigned kNamespaceIndex = 10;
  const scada::NodeId kTestTypeId{101, kNamespaceIndex};
  const scada::NodeId kTestRefTypeId{102, kNamespaceIndex};
  const scada::NodeId kTestNode1Id{1, kNamespaceIndex};
  const scada::NodeId kTestNode2Id{2, kNamespaceIndex};
  const scada::NodeId kTestNode3Id{3, kNamespaceIndex};
  const scada::NodeId kTestNode4Id{4, kNamespaceIndex};
  const scada::NodeId kTestNode5Id{5, kNamespaceIndex};
  const scada::NodeId kTestNode6Id{6, kNamespaceIndex};
  const scada::NodeId kTestProp1Id{301, kNamespaceIndex};
  const scada::NodeId kTestProp2Id{302, kNamespaceIndex};

  // RootFolder : FolderType
  //   TestNode1 : TestType {TestProp1}
  //   TestNode2 : TestType {TestProp1, TestProp2}
  //   TestNode3 : TestType
  //     TestNode4 : TestType (Organizes)
  //     TestNode5 : TestType (HasComponent)
  //       TestNode6 : TestType (Organizes)

  std::vector<scada::NodeState> nodes{
      {scada::id::RootFolder,
       scada::NodeClass::Object,
       scada::id::FolderType,  // type id
       {},                     // parent id
       {},                     // reference type id
       scada::NodeAttributes{}.set_browse_name("Root")},
      {
          kTestNode1Id,
          scada::NodeClass::Object,
          kTestTypeId,            // type id
          scada::id::RootFolder,  // parent id
          scada::id::Organizes,   // reference type id
          scada::NodeAttributes{}.set_browse_name("TestNode1"),
          {
              // properties
              {kTestProp1Id, "TestNode1.TestProp1.Value"},
          },
      },
      {
          kTestNode2Id,
          scada::NodeClass::Object,
          kTestTypeId,            // type id
          scada::id::RootFolder,  // parent id
          scada::id::Organizes,   // reference type id
          scada::NodeAttributes{}.set_browse_name("TestNode2"),
          {
              // properties
              {kTestProp1Id, "TestNode2.TestProp1.Value"},
              {kTestProp2Id, "TestNode2.TestProp2.Value"},
          },
      },
      {
          kTestNode3Id,
          scada::NodeClass::Object,
          kTestTypeId,            // type id
          scada::id::RootFolder,  // parent id
          scada::id::Organizes,   // reference type id
          scada::NodeAttributes{}.set_browse_name("TestNode3"),
      },
      {
          kTestNode4Id,
          scada::NodeClass::Object,
          kTestTypeId,           // type id
          kTestNode3Id,          // parent id
          scada::id::Organizes,  // reference type id
          scada::NodeAttributes{}.set_browse_name("TestNode4"),
      },
      {
          kTestNode5Id,
          scada::NodeClass::Object,
          kTestTypeId,              // type id
          kTestNode3Id,             // parent id
          scada::id::HasComponent,  // reference type id
          scada::NodeAttributes{}.set_browse_name("TestNode5"),
      },
      {
          kTestNode6Id,
          scada::NodeClass::Object,
          kTestTypeId,           // type id
          kTestNode5Id,          // parent id
          scada::id::Organizes,  // reference type id
          scada::NodeAttributes{}.set_browse_name("TestNode6"),
      },
  };

  std::vector<scada::ReferenceState> references{
      {kTestRefTypeId, kTestNode2Id, kTestNode3Id},
  };

 private:
  scada::DataValue ReadNode(const scada::ReadValueId& read_id) const;
  scada::DataValue ReadProperty(const scada::ReadValueId& read_id,
                                const scada::NodeId& child_id) const;

  scada::BrowseResult BrowseNode(
      const scada::BrowseDescription& description) const;
  scada::BrowseResult BrowseProperty(
      const scada::NodeId& parent_id,
      const scada::NodeId& child_id,
      const scada::BrowseDescription& description) const;

  scada::Variant GetPropertyValue(
      const scada::NodeState& node,
      const scada::NodeId& property_declaration_id) const;
  scada::QualifiedName GetPropertyName(
      const scada::NodeId& property_declaration_id) const;

  bool IsSubtype(const scada::NodeId& type_id,
                 const scada::NodeId& supertype_id) const;
  scada::NodeId GetSupertypeId(const scada::NodeId& type_id) const;

  static bool BrowseMatches(const scada::BrowseDescription& description,
                            bool forward);
  bool BrowseMatches(const scada::BrowseDescription& description,
                     const scada::NodeId& reference_type_id) const;
  bool BrowseMatches(const scada::BrowseDescription& description,
                     const scada::NodeId& reference_type_id,
                     bool forward) const;

  base::ObserverList<scada::ViewEvents> view_events_;
};

inline void TestAddressSpace::Read(
    const std::vector<scada::ReadValueId>& value_ids,
    const scada::ReadCallback& callback) {
  std::vector<scada::DataValue> results(value_ids.size());
  for (size_t i = 0; i < value_ids.size(); ++i)
    results[i] = Read(value_ids[i]);
  callback(scada::StatusCode::Good, std::move(results));
}

inline void TestAddressSpace::Write(const scada::NodeId& node_id,
                                    double value,
                                    const scada::NodeId& user_id,
                                    const scada::WriteFlags& flags,
                                    const scada::StatusCallback& callback) {
  assert(false);
}

inline void TestAddressSpace::Browse(
    const std::vector<scada::BrowseDescription>& nodes,
    const scada::BrowseCallback& callback) {
  std::vector<scada::BrowseResult> results(nodes.size());
  for (size_t i = 0; i < nodes.size(); ++i)
    results[i] = Browse(nodes[i]);
  callback(scada::StatusCode::Good, std::move(results));
}

inline scada::DataValue TestAddressSpace::Read(
    const scada::ReadValueId& read_id) const {
  scada::NodeId node_id;
  scada::NodeId child_id;
  if (IsNestedNodeId(read_id.node_id, node_id, child_id))
    return ReadProperty({node_id, read_id.attribute_id}, child_id);
  else
    return ReadNode(read_id);
}

inline scada::DataValue TestAddressSpace::ReadNode(
    const scada::ReadValueId& read_id) const {
  auto* node = GetNode(read_id.node_id);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};

  switch (read_id.attribute_id) {
    case scada::AttributeId::NodeClass:
      return scada::MakeReadResult(static_cast<int>(node->node_class));

    case scada::AttributeId::BrowseName:
      return scada::MakeReadResult(node->attributes.browse_name);
  }

  if (node->node_class == scada::NodeClass::Variable) {
    switch (read_id.attribute_id) {
      case scada::AttributeId::DataType:
        return scada::MakeReadResult(node->attributes.data_type);
    }
  }

  return {scada::StatusCode::Bad_WrongAttributeId, scada::DateTime::Now()};
}

inline scada::DataValue TestAddressSpace::ReadProperty(
    const scada::ReadValueId& read_id,
    const scada::NodeId& child_id) const {
  auto* node = GetNode(read_id.node_id);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};

  switch (read_id.attribute_id) {
    case scada::AttributeId::NodeClass:
      return scada::MakeReadResult(
          static_cast<int>(scada::NodeClass::Variable));

    case scada::AttributeId::BrowseName:
      return scada::MakeReadResult(GetPropertyName(child_id));

    case scada::AttributeId::DataType:
      return scada::MakeReadResult(scada::NodeId{scada::id::String});

    case scada::AttributeId::Value:
      return scada::MakeReadResult(GetPropertyValue(*node, child_id));
  }

  return {scada::StatusCode::Bad_WrongAttributeId, scada::DateTime::Now()};
}

inline scada::BrowseResult TestAddressSpace::Browse(
    const scada::BrowseDescription& description) const {
  scada::NodeId node_id;
  scada::NodeId child_id;
  if (IsNestedNodeId(description.node_id, node_id, child_id))
    return BrowseProperty(node_id, child_id, description);
  else
    return BrowseNode(description);
}

inline scada::BrowseResult TestAddressSpace::BrowseNode(
    const scada::BrowseDescription& description) const {
  scada::BrowseResult result;

  if (!description.include_subtypes) {
    assert(false);
    result.status_code = scada::StatusCode::Bad;
    return result;
  }

  auto* node = GetNode(description.node_id);
  if (!node) {
    assert(false);
    result.status_code = scada::StatusCode::Bad_WrongNodeId;
    return result;
  }

  std::vector<scada::ReferenceDescription> references;

  if (BrowseMatches(description, node->reference_type_id, false))
    references.push_back({node->reference_type_id, false, node->parent_id});

  if (BrowseMatches(description, true)) {
    for (auto& node : nodes) {
      if (node.parent_id == description.node_id &&
          BrowseMatches(description, node.reference_type_id)) {
        references.push_back({node.reference_type_id, true, node.node_id});
      }
    }
  }

  if (BrowseMatches(description, scada::id::HasProperty, true)) {
    for (auto& p : node->properties)
      references.push_back({scada::id::HasProperty, true,
                            MakeNestedNodeId(description.node_id, p.first)});
  }

  if (!node->type_definition_id.is_null() &&
      BrowseMatches(description, scada::id::HasTypeDefinition, true)) {
    references.push_back(
        {scada::id::HasTypeDefinition, true, node->type_definition_id});
  }

  for (auto& r : this->references) {
    if (r.source_id != description.node_id &&
        r.target_id != description.node_id)
      continue;

    bool forward = r.source_id == description.node_id;
    auto& target_id = forward ? r.target_id : r.source_id;
    if (BrowseMatches(description, r.reference_type_id, forward))
      references.push_back({r.reference_type_id, forward, target_id});
  }

  return {scada::StatusCode::Good, std::move(references)};
}

inline scada::BrowseResult TestAddressSpace::BrowseProperty(
    const scada::NodeId& parent_id,
    const scada::NodeId& child_id,
    const scada::BrowseDescription& description) const {
  scada::BrowseResult result;

  if (!description.include_subtypes) {
    assert(false);
    result.status_code = scada::StatusCode::Bad;
    return result;
  }

  auto* node = GetNode(parent_id);
  if (!node) {
    assert(false);
    result.status_code = scada::StatusCode::Bad_WrongNodeId;
    return result;
  }

  std::vector<scada::ReferenceDescription> references;

  if (description.direction == scada::BrowseDirection::Inverse) {
    references.push_back({scada::id::HasProperty, false, node->node_id});

  } else if (description.reference_type_id ==
             scada::id::HierarchicalReferences) {
  } else if (description.reference_type_id ==
             scada::id::NonHierarchicalReferences) {
    // Expect no type definition requests for now.
    // references.push_back({scada::id::HasTypeDefinition, true,
    // scada::id::PropertyType});
    assert(false);

  } else {
    assert(false);
    result.status_code = scada::StatusCode::Bad_WrongReferenceId;
    return result;
  }

  return {scada::StatusCode::Good, std::move(references)};
}

inline void TestAddressSpace::TranslateBrowsePath(
    const scada::NodeId& starting_node_id,
    const scada::RelativePath& relative_path,
    const scada::TranslateBrowsePathCallback& callback) {
  callback(scada::StatusCode::Bad, {}, 0);
}

inline scada::NodeState* TestAddressSpace::GetNode(
    const scada::NodeId& node_id) {
  for (auto& node : nodes) {
    if (node.node_id == node_id)
      return &node;
  }
  return nullptr;
}

inline const scada::NodeState* TestAddressSpace::GetNode(
    const scada::NodeId& node_id) const {
  return const_cast<TestAddressSpace*>(this)->GetNode(node_id);
}

inline void TestAddressSpace::DeleteNode(const scada::NodeId& node_id) {
  auto i = std::find_if(nodes.begin(), nodes.end(),
                        [node_id](const scada::NodeState& node) {
                          return node.node_id == node_id;
                        });
  if (i != nodes.end())
    nodes.erase(i);
}

inline void TestAddressSpace::NotifyModelChanged(
    const scada::ModelChangeEvent& event) {
  for (auto& e : view_events_)
    e.OnModelChanged(event);
}

inline void TestAddressSpace::NotifyNodeSemanticsChanged(
    const scada::NodeId& node_id) {
  for (auto& e : view_events_)
    e.OnNodeSemanticsChanged(node_id);
}

// static
inline scada::NodeId TestAddressSpace::MakeNestedNodeId(
    const scada::NodeId& parent_id,
    const scada::NodeId& child_id) {
  assert(parent_id.namespace_index() == child_id.namespace_index());
  assert(parent_id.type() == scada::NodeIdType::Numeric);
  assert(child_id.type() == scada::NodeIdType::Numeric);

  return scada::NodeId{parent_id.numeric_id() * 1000 + child_id.numeric_id(),
                       parent_id.namespace_index()};
}

// static
inline bool TestAddressSpace::IsNestedNodeId(const scada::NodeId& node_id,
                                             scada::NodeId& parent_id,
                                             scada::NodeId& child_id) {
  if (node_id.type() != scada::NodeIdType::Numeric)
    return false;

  if (node_id.numeric_id() < 1000)
    return false;

  parent_id =
      scada::NodeId{node_id.numeric_id() / 1000, node_id.namespace_index()};
  child_id =
      scada::NodeId{node_id.numeric_id() % 1000, node_id.namespace_index()};
  return true;
}

inline scada::Variant TestAddressSpace::GetPropertyValue(
    const scada::NodeState& node,
    const scada::NodeId& property_declaration_id) const {
  auto i =
      std::find_if(node.properties.begin(), node.properties.end(),
                   [&](auto& p) { return p.first == property_declaration_id; });
  if (i == node.properties.end())
    return {};
  return i->second;
}

inline scada::QualifiedName TestAddressSpace::GetPropertyName(
    const scada::NodeId& property_declaration_id) const {
  if (property_declaration_id == kTestProp1Id)
    return "TestProp1";
  else if (property_declaration_id == kTestProp2Id)
    return "TestProp2";
  else {
    assert(false);
    return {};
  }
}

inline scada::NodeId TestAddressSpace::GetSupertypeId(
    const scada::NodeId& type_id) const {
  static const std::map<scada::NodeId, scada::NodeId> kMap = {
      {scada::id::References, {}},
      {scada::id::HierarchicalReferences, scada::id::References},
      {scada::id::Organizes, scada::id::HierarchicalReferences},
      {scada::id::HasChild, scada::id::HierarchicalReferences},
      {scada::id::Aggregates, scada::id::HasChild},
      {scada::id::HasComponent, scada::id::Aggregates},
      {scada::id::HasProperty, scada::id::Aggregates},
      {scada::id::NonHierarchicalReferences, scada::id::References},
      {scada::id::HasTypeDefinition, scada::id::NonHierarchicalReferences},
      {kTestRefTypeId, scada::id::NonHierarchicalReferences},
  };

  auto i = kMap.find(type_id);
  if (i == kMap.end()) {
    assert(false);
    return false;
  }

  return i->second;
}

inline bool TestAddressSpace::IsSubtype(
    const scada::NodeId& type_id,
    const scada::NodeId& supertype_id) const {
  assert(!type_id.is_null());
  assert(!supertype_id.is_null());
  for (auto id = type_id; !id.is_null(); id = GetSupertypeId(id)) {
    if (id == supertype_id)
      return true;
  }
  return false;
}

// static
inline bool TestAddressSpace::BrowseMatches(
    const scada::BrowseDescription& description,
    bool forward) {
  if (description.direction == scada::BrowseDirection::Both)
    return true;
  bool f = description.direction == scada::BrowseDirection::Forward;
  return f == forward;
}

inline bool TestAddressSpace::BrowseMatches(
    const scada::BrowseDescription& description,
    const scada::NodeId& reference_type_id) const {
  if (!description.include_subtypes)
    return reference_type_id == description.reference_type_id;

  if (description.reference_type_id.is_null())
    return true;

  return IsSubtype(reference_type_id, description.reference_type_id);
}

inline bool TestAddressSpace::BrowseMatches(
    const scada::BrowseDescription& description,
    const scada::NodeId& reference_type_id,
    bool forward) const {
  return BrowseMatches(description, forward) &&
         BrowseMatches(description, reference_type_id);
}

}  // namespace test
