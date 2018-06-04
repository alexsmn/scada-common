#include "view_service_impl.h"

#include "base/logger.h"
#include "common/namespaces.h"
#include "common/node_id_util.h"
#include "core/attribute_service.h"
#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "core/standard_node_ids.h"

#include <gmock/gmock.h>

namespace {

class VirtualObject : public scada::GenericObject,
                      private scada::AttributeService,
                      private scada::ViewService {
 public:
  explicit VirtualObject(scada::NodeId id) { set_id(std::move(id)); }

  virtual AttributeService* GetAttributeService() final { return this; }
  virtual ViewService* GetViewService() final { return this; }

  virtual void Read(const std::vector<scada::ReadValueId>& value_ids,
                    const scada::ReadCallback& callback) override {
    std::vector<scada::DataValue> results(value_ids.size());
    for (size_t i = 0; i < value_ids.size(); ++i)
      results[i] = Read(value_ids[i]);
    callback(scada::StatusCode::Good, std::move(results));
  }

  virtual void Write(const scada::NodeId& node_id,
                     double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const scada::StatusCallback& callback) override {
    assert(false);
    callback(scada::StatusCode::Bad);
  }

  virtual void Browse(const std::vector<scada::BrowseDescription>& descriptions,
                      const scada::BrowseCallback& callback) override {
    std::vector<scada::BrowseResult> results(descriptions.size());
    for (size_t i = 0; i < descriptions.size(); ++i)
      results[i] = Browse(descriptions[i]);
    callback(scada::StatusCode::Good, std::move(results));
  }

  virtual void TranslateBrowsePath(
      const scada::NodeId& starting_node_id,
      const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback) override {
    callback(scada::StatusCode::Bad, {}, 0);
  }

  virtual void Subscribe(scada::ViewEvents& events) override {}
  virtual void Unsubscribe(scada::ViewEvents& events) override {}

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
    scada::AddReference(address_space, scada::id::HasTypeDefinition, kObjectId,
                        scada::id::BaseObjectType);

    auto& child =
        address_space.AddStaticNode(std::make_unique<VirtualObject>(kChildId));
    child.SetBrowseName(NodeIdToScadaString(kChildId));
    scada::AddReference(address_space, scada::id::HasTypeDefinition, kChildId,
                        scada::id::BaseObjectType);
    scada::AddReference(address_space, scada::id::HasComponent, kObjectId,
                        kChildId);
  }

  const std::shared_ptr<Logger> logger = std::make_shared<NullLogger>();
  AddressSpaceImpl address_space{logger};
  ViewServiceImpl view_service{{address_space}};

  // Node id-s must be correct to parse.
  const scada::NodeId kObjectId{23, NamespaceIndexes::GROUP};
  const scada::NodeId kChildId{43, NamespaceIndexes::TS};

  const std::vector<std::string> kItems{"Item1", "Item2", "Item3"};
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
  bool called = false;
  context.view_service.Browse(
      {{context.kObjectId, scada::BrowseDirection::Forward,
        scada::id::HierarchicalReferences, true}},
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
}

TEST(ViewServiceImpl, BrowseChildParent) {
  TestContext context;
  bool called = false;
  context.view_service.Browse(
      {{MakeNestedNodeId(context.kObjectId, context.kItems[0]),
        scada::BrowseDirection::Inverse, scada::id::HierarchicalReferences,
        true}},
      [&](scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        called = true;
        ASSERT_TRUE(status);
        ASSERT_EQ(1, results.size());
        auto& result = results.front();
        ASSERT_TRUE(scada::Status{result.status_code});
        auto& references = result.references;
        // Parent
        ASSERT_EQ(1, references.size());
        auto& reference = references[0];
        EXPECT_EQ(scada::NodeId{scada::id::Organizes},
                  reference.reference_type_id);
      });
  EXPECT_TRUE(called);
}
