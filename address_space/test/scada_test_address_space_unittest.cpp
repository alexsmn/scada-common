#include "address_space/test/scada_test_address_space.h"

#include "address_space/node_utils.h"
#include "model/data_items_node_ids.h"
#include "model/history_node_ids.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

namespace scada_test {

namespace {

using ::testing::NotNull;

// Every SCADA reference type this fixture defines must be a subtype of
// NonHierarchicalReferences. Node fetchers browse that supertype with
// include-subtypes, so an unparented reference type still resolves by node id
// while silently dropping its references from fetch results — a reference that
// exists and reads null. The consumers cannot add these themselves either:
// GenericNodeFactory cannot create a ReferenceType, so a type missing here is
// hand-rolled at the call site, which is how HasDevice came to be added twice
// and wired differently each time.
void ExpectNonHierarchicalReferenceType(
    const scada::AddressSpace& address_space,
    const scada::NodeId& node_id) {
  auto* reference_type = scada::AsReferenceType(address_space.GetNode(node_id));
  ASSERT_THAT(reference_type, NotNull());
  EXPECT_EQ(reference_type->GetNodeClass(), scada::NodeClass::ReferenceType);
  EXPECT_TRUE(scada::IsSubtypeOf(*reference_type,
                                 scada::id::NonHierarchicalReferences));
}

TEST(ScadaTestAddressSpaceTest, DefinesTheScadaReferenceTypesItsConsumersWire) {
  ScadaTestAddressSpace address_space;

  ExpectNonHierarchicalReferenceType(
      address_space, scada::data_items::id::HasSimulationSignal);
  ExpectNonHierarchicalReferenceType(address_space,
                                     scada::history::id::HasHistoricalDatabase);
  ExpectNonHierarchicalReferenceType(address_space,
                                     scada::data_items::id::HasTsFormat);
  ExpectNonHierarchicalReferenceType(address_space,
                                     scada::data_items::id::HasDevice);
}

}  // namespace

}  // namespace scada_test
