#include "node_service/node_util.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_impl2.h"
#include "address_space/address_space_util.h"
#include "address_space/address_space_xml.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "address_space/standard_address_space.h"
#include "address_space/test/scada_test_address_space.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "model/static_nodesets.h"
#include "node_service/test/create_test_node_service.h"

#include <gmock/gmock.h>

#include <vector>

#include "base/debug_util.h"

using namespace testing;

namespace {

std::vector<scada::NodeId> CreatableTypeIds(const NodeRef& node) {
  std::vector<scada::NodeId> ids;
  for (const auto& type : GetCreatableChildTypes(node))
    ids.push_back(type.node_id());
  return ids;
}

}  // namespace

// The NodeRef GetCreatableChildTypes reads the OptionalPlaceholder
// InstanceDeclarations that replaced the proprietary Creates reference. Backed
// by a synchronous StaticNodeService over the real static SCADA address space.
TEST(NodeUtil, GetCreatableChildTypesFromPlaceholders) {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};
  ASSERT_TRUE(scada::LoadStaticAddressSpace(
      scada::GetScadaStaticNodesetSourcePaths(), space, factory));
  auto node_service = node_service::test::CreateTestNodeService(space);

  // The DataItems folder (an instance of DataItemsFolderType) may organize
  // DataGroupType nodes via its <DataGroup> placeholder.
  EXPECT_THAT(
      CreatableTypeIds(node_service->GetNode(scada::data_items::id::DataItems)),
      UnorderedElementsAre(scada::data_items::id::DataGroupType));

  // DataGroupType itself declares nested DataGroups and DataItems.
  EXPECT_THAT(CreatableTypeIds(
                  node_service->GetNode(scada::data_items::id::DataGroupType)),
              UnorderedElementsAre(scada::data_items::id::DataGroupType,
                                   scada::data_items::id::DataItemType));

  // A leaf type with no placeholder declarations yields nothing.
  EXPECT_THAT(CreatableTypeIds(
                  node_service->GetNode(scada::data_items::id::DataItemType)),
              IsEmpty());
}

// LinkType derives from DeviceType in the nodeset, so a link IS a device — the
// relation GetFullDisplayName relies on to qualify a device by its parent link.
TEST(NodeUtil, TestAddressSpaceLinkTypeDerivesFromDeviceType) {
  scada_test::ScadaTestAddressSpace address_space;
  auto node_service = node_service::test::CreateTestNodeService(address_space);

  EXPECT_TRUE(IsSubtypeOf(node_service->GetNode(scada::devices::id::LinkType),
                          scada::devices::id::DeviceType));
  EXPECT_TRUE(
      IsSubtypeOf(node_service->GetNode(scada::devices::id::ModbusLinkType),
                  scada::devices::id::DeviceType));
}

TEST(NodeUtil, GetFullDisplayName_Iec61850Model) {
  scada_test::ScadaTestAddressSpace address_space;

  const scada::NodeId model_id{"1!Model",
                               scada::NamespaceIndexes::IEC61850_DEVICE};
  const scada::NodeId device_id{1, scada::NamespaceIndexes::IEC61850_DEVICE};

  address_space.AddStaticNode<scada::GenericObject>(model_id, "ModelBrowseName",
                                                    u"ModelDisplayName");
  address_space.AddStaticNode<scada::GenericObject>(
      device_id, "DeviceBrowseName", u"DeviceDisplayName");
  scada::AddReference(address_space, scada::id::Organizes,
                      scada::devices::id::Devices, device_id);
  scada::AddReference(address_space, scada::id::HasTypeDefinition, device_id,
                      scada::devices::id::Iec61850DeviceType);
  scada::AddReference(address_space, scada::id::Organizes, device_id, model_id);
  scada::AddReference(address_space, scada::id::HasTypeDefinition, model_id,
                      scada::devices::id::Iec61850LogicalNodeType);

  auto node_service = node_service::test::CreateTestNodeService(address_space);

  const std::u16string expected_full_display_name =
      u"DeviceDisplayName : ModelDisplayName";
  EXPECT_EQ(expected_full_display_name,
            GetFullDisplayName(node_service->GetNode(model_id)));
}
