#include "view_service_impl.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_type_system.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "address_space/standard_address_space.h"
#include "address_space/test/test_address_space.h"
#include "base/test/awaitable_test.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "scada/attribute_service.h"
#include "scada/service_context.h"
#include "scada/standard_node_ids.h"
#include "scada/test/status_matchers.h"

#include <gmock/gmock.h>

namespace {

class VirtualObject : public scada::GenericObject,
                      private scada::AttributeService,
                      private scada::ViewService {
 public:
  explicit VirtualObject(scada::NodeId id) { set_id(std::move(id)); }

  virtual AttributeService* GetAttributeService() final { return this; }
  virtual ViewService* GetViewService() final { return this; }

  Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> Read(
      scada::ServiceContext context,
      std::shared_ptr<const std::vector<scada::ReadValueId>> inputs) override {
    std::vector<scada::DataValue> results(inputs->size());
    for (size_t i = 0; i < inputs->size(); ++i)
      results[i] = Read((*inputs)[i]);
    co_return results;
  }

  Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> Write(
      scada::ServiceContext context,
      std::shared_ptr<const std::vector<scada::WriteValue>> inputs) override {
    assert(false);
    co_return scada::Status{scada::StatusCode::Bad};
  }

  Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>> Browse(
      scada::ServiceContext context,
      std::vector<scada::BrowseDescription> descriptions) override {
    std::vector<scada::BrowseResult> results(descriptions.size());
    for (size_t i = 0; i < descriptions.size(); ++i)
      results[i] = Browse(descriptions[i]);
    co_return results;
  }

  Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<scada::BrowsePath> browse_paths) override {
    co_return scada::Status{scada::StatusCode::Bad};
  }

  std::vector<std::string> items;

 private:
  scada::DataValue Read(const scada::ReadValueId& read_value_id) const {
    return {scada::StatusCode::Bad, scada::DateTime::Now()};
  }

  scada::BrowseResult Browse(
      const scada::BrowseDescription& description) const {
    scada::BrowseResult result;

    if (!description.include_subtypes ||
        description.reference_type_id != scada::id::HierarchicalReferences) {
      assert(false);
      result.status_code = scada::StatusCode::Bad_WrongNodeId;
      return result;
    }

    if (description.node_id == id()) {
      if (description.direction == scada::BrowseDirection::Forward ||
          description.direction == scada::BrowseDirection::Both) {
        for (auto& item : items)
          result.references.push_back(
              {scada::id::Organizes, true, MakeNestedNodeId(id(), item)});
      }

    } else {
      if (description.direction == scada::BrowseDirection::Inverse ||
          description.direction == scada::BrowseDirection::Both) {
        result.references.push_back({scada::id::Organizes, false, id()});
      }
    }

    return result;
  }
};

struct TestContext {
  TestContext() {
    auto& object =
        address_space.AddStaticNode(std::make_unique<VirtualObject>(kObjectId));
    object.SetBrowseName(NodeIdToScadaString(kObjectId));
    object.items = kItems;
    object_ = &object;
    scada::AddReference(address_space, scada::id::HasTypeDefinition, kObjectId,
                        scada::id::BaseObjectType);

    auto& child =
        address_space.AddStaticNode(std::make_unique<VirtualObject>(kChildId));
    child.SetBrowseName(NodeIdToScadaString(kChildId));
    child_ = &child;
    scada::AddReference(address_space, scada::id::HasTypeDefinition, kChildId,
                        scada::id::BaseObjectType);
    scada::AddReference(address_space, scada::id::HasComponent, kObjectId,
                        kChildId);
  }

  ~TestContext() {
    // AddressSpaceImpl::Clear() only tears down dynamically created nodes, not
    // AddStaticNode'd ones, so the static nodes' references must be removed here
    // — while StandardAddressSpace's reference-type / base-type nodes are still
    // alive — to avoid dangling pointers during member destruction.
    scada::DeleteAllReferences(address_space, *child_);
    scada::DeleteAllReferences(address_space, *object_);
  }

  AddressSpaceImpl address_space;
  // Populate the standard reference-type / base-type hierarchy so reference
  // subtype filtering during Browse resolves (a bare address space would have
  // unresolved reference types).
  StandardAddressSpace standard_address_space{address_space};
  AddressSpaceTypeSystem type_system{address_space};
  SyncViewServiceImpl sync_view_service{{address_space}};
  ViewServiceImpl view_service{sync_view_service};

  // Node id-s must be correct to parse.
  const scada::NodeId kObjectId{23, NamespaceIndexes::GROUP};
  const scada::NodeId kChildId{43, NamespaceIndexes::TS};

  const std::vector<std::string> kItems{"Item1", "Item2", "Item3"};

  VirtualObject* object_ = nullptr;
  VirtualObject* child_ = nullptr;
};

}  // namespace

/*TEST(ViewServiceImpl, ReadChild) {
  TestContext context;
  bool called = false;
  context.view_service.Read({MakeNestedNodeId(context.kObjectId,
context.kItems[0]), scada::AttributeId::BrowseName},
      [&](scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        called = true;
        ASSERT_TRUE(status);
        ASSERT_EQ(1, results.size());
        auto& result = results.front();
        ASSERT_TRUE(scada::Status{result.status_code});
        auto& references = result.references;
        // Child + Items
        ASSERT_EQ(1 + context.kItems.size(), references.size());
      });
  EXPECT_TRUE(called);
}*/

TEST(ViewServiceImpl, BrowseParentChildren) {
  TestContext context;
  TestExecutor executor;
  ASSERT_OK_AND_ASSIGN(
      auto results,
      WaitAwaitable(
          executor,
          context.view_service.Browse(
              scada::ServiceContext{},
              {{context.kObjectId, scada::BrowseDirection::Forward,
                scada::id::HierarchicalReferences, true}})));
  ASSERT_EQ(static_cast<size_t>(1), results.size());
  auto& result = results.front();
  ASSERT_TRUE(scada::Status{result.status_code});
  // The hierarchical (HasComponent) child is returned; the non-hierarchical
  // HasTypeDefinition reference is excluded by the reference-type filter.
  ASSERT_EQ(result.references.size(), 1u);
  EXPECT_EQ(result.references[0].node_id, context.kChildId);
  EXPECT_TRUE(result.references[0].forward);
  EXPECT_EQ(result.references[0].reference_type_id,
            scada::NodeId{scada::id::HasComponent});
}

TEST(ViewServiceImpl, BrowsePopulatesNamesAndHonorsResultMask) {
  TestAddressSpace address_space;
  TestExecutor executor;

  const auto browse_test_node1 = [&](scada::UInt32 result_mask) {
    auto results = WaitAwaitable(
        executor,
        address_space.view_service_impl.Browse(
            scada::ServiceContext{},
            {{.node_id = scada::id::RootFolder,
              .direction = scada::BrowseDirection::Forward,
              .reference_type_id = scada::id::HierarchicalReferences,
              .include_subtypes = true,
              .result_mask = result_mask}}));
    EXPECT_TRUE(results.ok());
    std::optional<scada::ReferenceDescription> match;
    if (results.ok() && results->size() == 1u) {
      for (const auto& reference : (*results)[0].references) {
        if (reference.node_id == address_space.kTestNode1Id)
          match = reference;
      }
    }
    return match;
  };

  // Full result mask: BrowseName / DisplayName / NodeClass are populated.
  const auto full = browse_test_node1(0x3F);
  ASSERT_TRUE(full.has_value());
  EXPECT_EQ(full->browse_name.name(), "TestNode1");
  EXPECT_FALSE(full->display_name.empty());
  EXPECT_EQ(full->node_class, scada::NodeClass::Object);

  // NodeClass-only mask: name fields are left empty, NodeClass still set.
  const auto class_only = browse_test_node1(scada::kBrowseResultNodeClass);
  ASSERT_TRUE(class_only.has_value());
  EXPECT_TRUE(class_only->browse_name.empty());
  EXPECT_TRUE(class_only->display_name.empty());
  EXPECT_EQ(class_only->node_class, scada::NodeClass::Object);
}

TEST(ViewServiceImpl, BrowseAppliesNodeClassMask) {
  TestAddressSpace address_space;
  TestExecutor executor;

  const auto browse = [&](scada::UInt32 node_class_mask) {
    return WaitAwaitable(
        executor,
        address_space.view_service_impl.Browse(
            scada::ServiceContext{},
            {{.node_id = scada::id::RootFolder,
              .direction = scada::BrowseDirection::Forward,
              .reference_type_id = scada::id::HierarchicalReferences,
              .include_subtypes = true,
              .node_class_mask = node_class_mask}}));
  };

  // No mask: the Object node TestNode1 is among the children.
  ASSERT_OK_AND_ASSIGN(const auto all, browse(0));
  ASSERT_EQ(all.size(), 1u);
  EXPECT_THAT(all[0].references,
              testing::Contains(testing::Field(
                  &scada::ReferenceDescription::node_id,
                  address_space.kTestNode1Id)));

  // Object mask keeps the Object node.
  ASSERT_OK_AND_ASSIGN(
      const auto objects,
      browse(static_cast<scada::UInt32>(scada::NodeClass::Object)));
  ASSERT_EQ(objects.size(), 1u);
  EXPECT_THAT(objects[0].references,
              testing::Contains(testing::Field(
                  &scada::ReferenceDescription::node_id,
                  address_space.kTestNode1Id)));

  // Variable mask filters out the (Object-class) children.
  ASSERT_OK_AND_ASSIGN(
      const auto variables,
      browse(static_cast<scada::UInt32>(scada::NodeClass::Variable)));
  ASSERT_EQ(variables.size(), 1u);
  EXPECT_THAT(variables[0].references,
              testing::Not(testing::Contains(testing::Field(
                  &scada::ReferenceDescription::node_id,
                  address_space.kTestNode1Id))));
}

TEST(ViewServiceImpl, CoroutineBrowseReturnsSyncResults) {
  TestAddressSpace address_space;
  TestExecutor executor;

  ASSERT_OK_AND_ASSIGN(auto results,
                       WaitAwaitable(executor,
                                     address_space.view_service_impl.Browse(
                                         scada::ServiceContext{},
                                         {{scada::id::RootFolder,
                                           scada::BrowseDirection::Forward,
                                           scada::id::HierarchicalReferences,
                                           true}})));
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].status_code, scada::StatusCode::Good);
  EXPECT_THAT(results[0].references,
              testing::Contains(testing::Field(
                  &scada::ReferenceDescription::node_id,
                  address_space.kTestNode1Id)));
  EXPECT_THAT(
      results[0].references,
      testing::Contains(testing::AllOf(
          testing::Field(&scada::ReferenceDescription::node_id,
                         address_space.kTestNode1Id),
          testing::Field(&scada::ReferenceDescription::node_class,
                         scada::NodeClass::Object))));
}

TEST(ViewServiceImpl, CoroutineTranslateBrowsePathsReturnsSyncResults) {
  TestAddressSpace address_space;
  TestExecutor executor;

  ASSERT_OK_AND_ASSIGN(auto results, WaitAwaitable(
      executor, address_space.view_service_impl.TranslateBrowsePaths(
                    {{.node_id = scada::id::RootFolder,
                      .relative_path = {{.reference_type_id =
                                             scada::id::HierarchicalReferences,
                                         .include_subtypes = true,
                                         .target_name =
                                             scada::QualifiedName{
                                                 "TestNode1",
                                                 TestAddressSpace::
                                                     kNamespaceIndex}}}}})));
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].status_code, scada::StatusCode::Good);
  ASSERT_EQ(results[0].targets.size(), 1u);
  EXPECT_EQ(results[0].targets[0].target_id.node_id(),
            address_space.kTestNode1Id);
}

TEST(ViewServiceImpl, BrowseChildParent) {
  TestContext context;
  TestExecutor executor;
  ASSERT_OK_AND_ASSIGN(
      auto results,
      WaitAwaitable(executor,
                    context.view_service.Browse(
                        scada::ServiceContext{},
                        {{context.kChildId, scada::BrowseDirection::Inverse,
                          scada::id::HierarchicalReferences, true}})));
  ASSERT_EQ(static_cast<size_t>(1), results.size());
  auto& result = results.front();
  ASSERT_TRUE(scada::Status{result.status_code});
  // The inverse HasComponent reference points back to the parent.
  ASSERT_EQ(result.references.size(), 1u);
  auto& reference = result.references[0];
  EXPECT_EQ(reference.node_id, context.kObjectId);
  EXPECT_FALSE(reference.forward);
  EXPECT_EQ(reference.reference_type_id,
            scada::NodeId{scada::id::HasComponent});
}
