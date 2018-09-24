#pragma once

#include "address_space/address_space_impl.h"
#include "address_space/attribute_service_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node.h"
#include "address_space/node_utils.h"
#include "address_space/view_service_impl.h"
#include "base/observer_list.h"
#include "common/namespaces.h"
#include "common/node_id_util.h"
#include "common/node_state.h"
#include "core/attribute_service.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/view_service.h"

namespace testing {

class TestAddressSpace : public AddressSpaceImpl2,
                         public AttributeServiceImpl,
                         public ViewServiceImpl {
 public:
  TestAddressSpace();

  scada::NodeId MakeNestedNodeId(const scada::NodeId& parent_id,
                                 const scada::NodeId& component_decl_id);

  void CreateNode(const scada::NodeState& node_state);
  void DeleteNode(const scada::NodeId& node_id);

  // RootFolder : FolderType
  //   TestNode1 : TestType {TestProp1}
  //   TestNode2 : TestType {TestProp1, TestProp2}
  //   TestNode3 : TestType
  //     TestNode4 : TestType (Organizes)
  //     TestNode5 : TestType (HasComponent)
  //       TestNode6 : TestType (Organizes)

  static const unsigned kNamespaceIndex = NamespaceIndexes::SCADA;
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
  const char* kTestProp1BrowseName = "TestProp1";
  const char* kTestProp2BrowseName = "TestProp2";
};

inline TestAddressSpace::TestAddressSpace()
    : AddressSpaceImpl2{std::make_shared<NullLogger>()},
      AttributeServiceImpl{{*static_cast<scada::AddressSpace*>(this)}},
      ViewServiceImpl{{*static_cast<scada::AddressSpace*>(this)}} {
  GenericNodeFactory node_factory{*this};

  std::vector<scada::NodeState> nodes{
      {kTestRefTypeId,
       scada::NodeClass::ReferenceType,
       {},                                    // type id
       scada::id::NonHierarchicalReferences,  // parent id
       scada::id::HasSubtype,                 // reference type id
       scada::NodeAttributes{}.set_browse_name("TestRefType")},
      {kTestTypeId,
       scada::NodeClass::ObjectType,
       {},                         // type id
       scada::id::BaseObjectType,  // parent id
       scada::id::HasSubtype,      // reference type id
       scada::NodeAttributes{}.set_browse_name("TestType")},
      {kTestProp1Id, scada::NodeClass::Variable,
       scada::id::PropertyType,  // type id
       kTestTypeId,              // parent id
       scada::id::HasProperty,   // reference type id
       scada::NodeAttributes{}
           .set_browse_name(kTestProp1BrowseName)
           .set_data_type(scada::id::String)},
      {kTestProp2Id, scada::NodeClass::Variable,
       scada::id::PropertyType,  // type id
       kTestTypeId,              // parent id
       scada::id::HasProperty,   // reference type id
       scada::NodeAttributes{}
           .set_browse_name(kTestProp2BrowseName)
           .set_data_type(scada::id::String)},
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

  for (auto& node : nodes) {
    node.attributes.display_name =
        scada::ToLocalizedText(node.attributes.browse_name.name());
    node_factory.CreateNode(node);
  }

  for (auto& reference : references) {
    scada::AddReference(*this, reference.reference_type_id, reference.source_id,
                        reference.target_id);
  }
}

inline scada::NodeId TestAddressSpace::MakeNestedNodeId(
    const scada::NodeId& parent_id,
    const scada::NodeId& component_decl_id) {
  auto* component_decl = GetNode(component_decl_id);
  assert(component_decl);
  return ::MakeNestedNodeId(parent_id, component_decl->GetBrowseName().name());
}

inline void TestAddressSpace::CreateNode(const scada::NodeState& node_state) {
  GenericNodeFactory node_factory{*this};
  auto p = node_factory.CreateNode(node_state);
  assert(p.first);
  assert(p.second);
  NotifyModelChanged(
      {node_state.node_id, {}, scada::ModelChangeEvent::NodeAdded});
}

inline void TestAddressSpace::DeleteNode(const scada::NodeId& node_id) {
  assert(GetNode(node_id));
  RemoveNode(node_id);
  // NotifyModelChanged({node_id, {}, scada::ModelChangeEvent::NodeDeleted});
}

}  // namespace testing
