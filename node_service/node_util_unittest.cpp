#include "node_service/node_util.h"

#include "address_space/address_space_impl3.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "node_service/node_model_mock.h"
#include "node_service/v1/test/test_node_service.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

TEST(NodeUtil, GetFullDisplayName_Iec61850Model) {
  AddressSpaceImpl3 address_space;

  const scada::NodeId model_id{"1!Model", NamespaceIndexes::IEC61850_DEVICE};
  const scada::NodeId device_id{1, NamespaceIndexes::IEC61850_DEVICE};

  address_space.AddStaticNode<scada::GenericObject>(model_id, "ModelBrowseName",
                                                    u"ModelDisplayName");
  address_space.AddStaticNode<scada::GenericObject>(
      device_id, "DeviceBrowseName", u"DeviceDisplayName");
  scada::AddReference(address_space, scada::id::Organizes, devices::id::Devices,
                      device_id);
  scada::AddReference(address_space, scada::id::HasTypeDefinition, device_id,
                      devices::id::Iec61850DeviceType);
  scada::AddReference(address_space, scada::id::Organizes, device_id, model_id);
  scada::AddReference(address_space, scada::id::HasTypeDefinition, model_id,
                      devices::id::Iec61850LogicalNodeType);

  auto node_service = v1::CreateTestNodeService(address_space);

  const std::u16string expected_full_display_name =
      u"DeviceDisplayName : ModelDisplayName";
  EXPECT_EQ(expected_full_display_name,
            GetFullDisplayName(node_service->GetNode(model_id)));
}
