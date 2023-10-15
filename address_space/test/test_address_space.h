#pragma once

#include "address_space/address_space_impl.h"
#include "address_space/address_space_type_system.h"
#include "address_space/address_space_util.h"
#include "address_space/attribute_service_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node.h"
#include "address_space/standard_address_space.h"
#include "address_space/view_service_impl.h"
#include "base/observer_list.h"
#include "common/node_state.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "scada/attribute_service.h"
#include "scada/attribute_service_mock.h"
#include "scada/node_class.h"
#include "scada/standard_node_ids.h"
#include "scada/view_service.h"
#include "scada/view_service_mock.h"

class TestAddressSpace : public AddressSpaceImpl,
                         public scada::MockAttributeService,
                         public scada::MockViewService {
 public:
  TestAddressSpace();
  ~TestAddressSpace();

  scada::NodeId MakeNestedNodeId(const scada::NodeId& parent_id,
                                 const scada::NodeId& component_decl_id);

  void CreateNode(const scada::NodeState& node_state);
  virtual void DeleteNode(const scada::NodeId& node_id) override;
  void AddReference(const scada::NodeId& reference_type_id,
                    const scada::NodeId& source_id,
                    const scada::NodeId& target_id);

  StandardAddressSpace standard_address_space{*this};

  AddressSpaceTypeSystem type_system{*this};

  SyncAttributeServiceImpl sync_attribute_service_impl{
      {*static_cast<scada::AddressSpace*>(this)}};
  AttributeServiceImpl attribute_service_impl{sync_attribute_service_impl};

  SyncViewServiceImpl sync_view_service_impl{
      {*static_cast<scada::AddressSpace*>(this)}};
  ViewServiceImpl view_service_impl{sync_view_service_impl};

  // RootFolder : FolderType
  //   TestNode1 : TestType {TestProp1, TestProp2}
  //   TestNode2 : TestType {TestProp1, TestProp2}
  //   TestNode3 : TestType {TestProp1, TestProp2}
  //     TestNode4 : TestType (Organizes)
  //     TestNode5 : TestType (HasComponent)
  //       TestNode6 : TestType (Organizes)

  static const unsigned kNamespaceIndex = NamespaceIndexes::SCADA;
  // TODO: Create a custom reference type.
  const scada::NodeId kTestTypeId{101, kNamespaceIndex};
  const scada::NodeId kTestReferenceTypeId{102, kNamespaceIndex};
  const scada::NodeId kTestNode1Id{1, kNamespaceIndex};
  const scada::NodeId kTestNode2Id{2, kNamespaceIndex};
  const scada::NodeId kTestNode3Id{3, kNamespaceIndex};
  const scada::NodeId kTestNode4Id{4, kNamespaceIndex};
  const scada::NodeId kTestNode5Id{5, kNamespaceIndex};
  const scada::NodeId kTestNode6Id{6, kNamespaceIndex};
  const scada::NodeId kTestProp1Id{301, kNamespaceIndex};
  const scada::NodeId kTestProp2Id{302, kNamespaceIndex};
  const std::string_view kTestProp1BrowseName = "TestProp1";
  const std::string_view kTestProp2BrowseName = "TestProp2";
};

inline TestAddressSpace::TestAddressSpace() {
  using namespace testing;

  ON_CALL(*this, Read(_, _, _))
      .WillByDefault(
          Invoke(&attribute_service_impl, &scada::AttributeService::Read));
  ON_CALL(*this, Write(_, _, _))
      .WillByDefault(
          Invoke(&attribute_service_impl, &scada::AttributeService::Write));

  ON_CALL(*this, Browse(_, _))
      .WillByDefault(Invoke(&view_service_impl, &scada::ViewService::Browse));
  ON_CALL(*this, TranslateBrowsePaths(_, _))
      .WillByDefault(Invoke(&view_service_impl,
                            &scada::ViewService::TranslateBrowsePaths));

  GenericNodeFactory node_factory{*this};

  // TODO: Load from file.
  std::vector<scada::NodeState> nodes{
      {kTestReferenceTypeId,
       scada::NodeClass::ReferenceType,
       {},                                    // type id
       scada::id::NonHierarchicalReferences,  // parent id
       scada::id::HasSubtype,                 // reference type id
       scada::NodeAttributes{}
           .set_browse_name("TestRefType")
           .set_display_name(u"TestRefTypeDisplayName")},
      {kTestTypeId,
       scada::NodeClass::ObjectType,
       {},                         // type id
       scada::id::BaseObjectType,  // parent id
       scada::id::HasSubtype,      // reference type id
       scada::NodeAttributes{}
           .set_browse_name("TestType")
           .set_display_name(u"TestTypeDisplayName")},
      {kTestProp1Id, scada::NodeClass::Variable,
       scada::id::PropertyType,  // type id
       kTestTypeId,              // parent id
       scada::id::HasProperty,   // reference type id
       scada::NodeAttributes{}
           .set_browse_name(std::string{kTestProp1BrowseName})
           .set_display_name(u"TestProp1DisplayName")
           .set_data_type(scada::id::String)},
      {kTestProp2Id, scada::NodeClass::Variable,
       scada::id::PropertyType,  // type id
       kTestTypeId,              // parent id
       scada::id::HasProperty,   // reference type id
       scada::NodeAttributes{}
           .set_browse_name(std::string{kTestProp2BrowseName})
           .set_display_name(u"TestProp2DisplayName")
           .set_data_type(scada::id::String)},
      {
          kTestNode1Id,
          scada::NodeClass::Object,
          kTestTypeId,            // type id
          scada::id::RootFolder,  // parent id
          scada::id::Organizes,   // reference type id
          scada::NodeAttributes{}
              .set_browse_name("TestNode1")
              .set_display_name(u"TestNode1DisplayName"),
          {
              // properties
              {kTestProp1Id, "TestNode1.TestProp1.Value"},
              {kTestProp2Id, "TestNode1.TestProp2.Value"},
          },
      },
      {
          kTestNode2Id,
          scada::NodeClass::Object,
          kTestTypeId,            // type id
          scada::id::RootFolder,  // parent id
          scada::id::Organizes,   // reference type id
          scada::NodeAttributes{}
              .set_browse_name("TestNode2")
              .set_display_name(u"TestNode2DisplayName"),
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
          scada::NodeAttributes{}
              .set_browse_name("TestNode3")
              .set_display_name(u"TestNode3DisplayName"),
      },
      {
          kTestNode4Id,
          scada::NodeClass::Object,
          kTestTypeId,           // type id
          kTestNode3Id,          // parent id
          scada::id::Organizes,  // reference type id
          scada::NodeAttributes{}
              .set_browse_name("TestNode4")
              .set_display_name(u"TestNode4DisplayName"),
      },
      {
          kTestNode5Id,
          scada::NodeClass::Object,
          kTestTypeId,              // type id
          kTestNode3Id,             // parent id
          scada::id::HasComponent,  // reference type id
          scada::NodeAttributes{}
              .set_browse_name("TestNode5")
              .set_display_name(u"TestNode5DisplayName"),
      },
      {
          kTestNode6Id,
          scada::NodeClass::Object,
          kTestTypeId,           // type id
          kTestNode5Id,          // parent id
          scada::id::Organizes,  // reference type id
          scada::NodeAttributes{}
              .set_browse_name("TestNode6")
              .set_display_name(u"TestNode6DisplayName"),
      },
  };

  std::vector<scada::ReferenceState> references{
      {kTestReferenceTypeId, kTestNode2Id, kTestNode3Id},
  };

  for (auto& node : nodes)
    node_factory.CreateNode(node);

  for (auto& reference : references) {
    scada::AddReference(*this, reference.reference_type_id, reference.source_id,
                        reference.target_id);
  }
}

inline TestAddressSpace::~TestAddressSpace() {
  Clear();
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
  ALLOW_UNUSED_LOCAL(p);
  assert(p.first);
  assert(p.second);
}

inline void TestAddressSpace::DeleteNode(const scada::NodeId& node_id) {
  assert(GetNode(node_id));
  AddressSpaceImpl::DeleteNode(node_id);
}

inline void TestAddressSpace::AddReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id) {
  scada::AddReference(*this, reference_type_id, source_id, target_id);
}
