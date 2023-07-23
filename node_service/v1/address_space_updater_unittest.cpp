#include "node_service/v1/address_space_updater.h"

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_factory_mock.h"
#include "address_space/object.h"
#include "address_space/standard_address_space.h"
#include "address_space/type_definition.h"
#include "base/range_util.h"
#include "common/node_state.h"
#include "scada/standard_node_ids.h"

#include <boost/range/adaptor/transformed.hpp>
#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

namespace {

std::vector<scada::ReferenceDescription> CollectReferenceDescriptions(
    const scada::References& references,
    bool forward) {
  return references |
         boost::adaptors::transformed(
             [forward](const scada::Reference& reference) {
               return scada::ReferenceDescription{reference.type->id(), forward,
                                                  reference.node->id()};
             }) |
         to_vector;
}

auto CollectReferenceDescriptions(const scada::Node& node) {
  return Join({CollectReferenceDescriptions(node.forward_references(), true),
               CollectReferenceDescriptions(node.inverse_references(), false)});
}

}  // namespace

class AddressSpaceUpdaterTest : public Test {
 public:
  AddressSpaceUpdaterTest();
  ~AddressSpaceUpdaterTest();

 protected:
  AddressSpaceImpl address_space_;
  StandardAddressSpace standard_address_space_{address_space_};

  GenericNodeFactory node_factory_impl_{address_space_};
  MockNodeFactory node_factory_;

  AddressSpaceUpdater address_space_updater_{address_space_, node_factory_};
};

AddressSpaceUpdaterTest::AddressSpaceUpdaterTest() {
  ON_CALL(node_factory_, CreateNode(_))
      .WillByDefault(Invoke(&node_factory_impl_, &NodeFactory::CreateNode));
}

AddressSpaceUpdaterTest::~AddressSpaceUpdaterTest() {
  address_space_.Clear();
}

TEST_F(AddressSpaceUpdaterTest, UpdateNodes_UpdateObject) {
  const auto node_state =
      scada::NodeState{}
          .set_node_id({1, 1})
          .set_node_class(scada::NodeClass::Object)
          .set_type_definition_id(scada::id::BaseObjectType)
          .set_parent(scada::id::Organizes, scada::id::RootFolder);

  EXPECT_CALL(node_factory_, CreateNode(_));

  address_space_updater_.UpdateNodes({node_state});
  address_space_updater_.UpdateNodes({node_state});

  auto* node = address_space_.GetNode(node_state.node_id);

  ASSERT_TRUE(node);
  EXPECT_EQ(node->GetNodeClass(), node_state.node_class);
  ASSERT_TRUE(node->type_definition());
  EXPECT_EQ(node->type_definition()->id(), scada::id::BaseObjectType);

  // Validate no duplicate references were added.

  EXPECT_THAT(CollectReferenceDescriptions(*node),
              UnorderedElementsAre(
                  scada::ReferenceDescription{scada::id::HasTypeDefinition,
                                              true, scada::id::BaseObjectType},
                  scada::ReferenceDescription{scada::id::Organizes, false,
                                              scada::id::RootFolder}));
}

TEST_F(AddressSpaceUpdaterTest, UpdateNodes_UpdateReferenceType) {
  const auto node_state =
      scada::NodeState{}
          .set_node_id({1, 1})
          .set_node_class(scada::NodeClass::ReferenceType)
          .set_supertype_id(scada::id::NonHierarchicalReferences);

  EXPECT_CALL(node_factory_, CreateNode(_));

  address_space_updater_.UpdateNodes({node_state});
  address_space_updater_.UpdateNodes({node_state});

  auto* node = address_space_.GetNode(node_state.node_id);

  ASSERT_TRUE(node);
  EXPECT_EQ(node->GetNodeClass(), node_state.node_class);
  EXPECT_FALSE(node->type_definition());

  // Validate no duplicate references were added.

  EXPECT_THAT(
      CollectReferenceDescriptions(*node),
      UnorderedElementsAre(scada::ReferenceDescription{
          scada::id::HasSubtype, false, scada::id::NonHierarchicalReferences}));
}
