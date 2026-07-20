#include "address_space/node_factory_util.h"

#include "address_space/address_space_impl2.h"
#include "address_space/address_space_xml.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/static_nodesets.h"

#include <gtest/gtest.h>

namespace scada {
namespace {

// Instantiating a type must materialize its Mandatory data variables but never
// its OptionalPlaceholder/MandatoryPlaceholder InstanceDeclarations. ModbusDeviceType
// carries the <TransmissionItem> OptionalPlaceholder (attached via
// HasTransmissionItem, a HasComponent subtype, so it is returned by
// GetComponents); the factory must skip it.
TEST(NodeFactoryUtil, SkipsPlaceholderDeclarations) {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};
  ASSERT_TRUE(LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(), space,
                                     factory));

  const auto* modbus_device_type =
      AsTypeDefinition(space.GetNode(devices::id::ModbusDeviceType));
  ASSERT_TRUE(modbus_device_type);

  // Materialize a device instance under the Devices folder, then populate its
  // data variables from the type — the path that would instantiate placeholders.
  const NodeId instance_id{devices::id::ModbusDeviceType.numeric_id() + 100000,
                           devices::id::ModbusDeviceType.namespace_index()};
  auto [instance_status, instance] = factory.CreateNode(
      NodeState{instance_id, NodeClass::Object, devices::id::ModbusDeviceType,
                devices::id::Devices, id::Organizes,
                NodeAttributes{}.set_browse_name(
                    QualifiedName{"TestModbusDevice"})});
  ASSERT_TRUE(instance_status);

  const Status status =
      CreateDataVariables(factory, instance_id, *modbus_device_type);
  ASSERT_TRUE(status);

  // The <TransmissionItem> placeholder must NOT be materialized as a child.
  EXPECT_FALSE(space.GetNode(MakeNestedNodeId(instance_id, "<TransmissionItem>")))
      << "the OptionalPlaceholder was wrongly instantiated";

  // A real Mandatory data variable (from the DeviceType supertype) IS created.
  EXPECT_TRUE(space.GetNode(MakeNestedNodeId(instance_id, "SyncClockCount")));
}

}  // namespace
}  // namespace scada
