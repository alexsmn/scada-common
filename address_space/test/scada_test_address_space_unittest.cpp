#include "address_space/test/scada_test_address_space.h"

#include "address_space/node_utils.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/history_node_ids.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

namespace scada_test {

namespace {

using ::testing::Contains;
using ::testing::Field;
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

// The hardware tree's root must offer what the nodeset offers there: a link of
// any kind and an IEC 61850 device. Until 2026-09-26 the fixture typed the
// folder with plain FolderType, so nothing was creatable at the root and the
// offline screenshot of its «Создать» menu (devices-create.png) had nothing to
// show; an IEC 60870 link likewise had no <Device> placeholder.
TEST(ScadaTestAddressSpaceTest, TheHardwareRootOffersLinksAndIec61850Devices) {
  ScadaTestAddressSpace address_space;
  namespace dev = scada::devices::id;

  const scada::Node* devices = address_space.GetNode(dev::Devices);
  ASSERT_THAT(devices, NotNull());
  const std::vector<scada::CreatableChildType> at_root =
      scada::GetCreatableChildTypes(*devices);
  EXPECT_THAT(at_root,
              Contains(Field(&scada::CreatableChildType::type_definition_id,
                             dev::LinkType)));
  EXPECT_THAT(at_root,
              Contains(Field(&scada::CreatableChildType::type_definition_id,
                             dev::Iec61850DeviceType)));

  const scada::Node* iec60870_link =
      address_space.GetNode(dev::Iec60870LinkType);
  ASSERT_THAT(iec60870_link, NotNull());
  EXPECT_THAT(scada::GetCreatableChildTypes(*iec60870_link),
              Contains(Field(&scada::CreatableChildType::type_definition_id,
                             dev::Iec60870DeviceType)));
}

}  // namespace

}  // namespace scada_test
