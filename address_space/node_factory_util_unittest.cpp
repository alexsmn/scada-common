#include "address_space/node_factory_util.h"

#include "address_space/address_space_impl2.h"
#include "address_space/address_space_xml.h"
#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/static_nodesets.h"

#include <gtest/gtest.h>

namespace scada {
namespace {

// Instantiating a type must materialize its Mandatory data variables but never
// its OptionalPlaceholder/MandatoryPlaceholder InstanceDeclarations.
// ModbusDeviceType carries the <TransmissionItem> OptionalPlaceholder (attached
// via HasTransmissionItem, a HasComponent subtype, so it is returned by
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
  // data variables from the type — the path that would instantiate
  // placeholders.
  const NodeId instance_id{devices::id::ModbusDeviceType.numeric_id() + 100000,
                           devices::id::ModbusDeviceType.namespace_index()};
  auto [instance_status, instance] = factory.CreateNode(NodeState{
      instance_id, NodeClass::Object, devices::id::ModbusDeviceType,
      devices::id::Devices, id::Organizes,
      NodeAttributes{.browse_name = QualifiedName{"TestModbusDevice"}}});
  ASSERT_TRUE(instance_status);

  const Status status =
      CreateMissingChildren(factory, instance_id, *modbus_device_type);
  ASSERT_TRUE(status);

  // The <TransmissionItem> placeholder must NOT be materialized as a child.
  EXPECT_FALSE(
      space.GetNode(MakeNestedNodeId(instance_id, "<TransmissionItem>")))
      << "the OptionalPlaceholder was wrongly instantiated";

  // A real Mandatory data variable (from the DeviceType supertype) IS created.
  EXPECT_TRUE(space.GetNode(MakeNestedNodeId(instance_id, "SyncClockCount")));
}


// The reason CreateDataVariables had to go: it did `AsVariable(node)` and
// `Check(decl.type_definition())` on every HasComponent child. A Method has
// neither, so pointing the old code at a type carrying one aborted the process.
// DataItemControlType carries three.
TEST(NodeFactoryUtil, MaterializesMethodComponentsWithoutCheckFailure) {
  AddressSpaceImpl2 space;
  GenericNodeFactory factory{space};
  ASSERT_TRUE(LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(), space,
                                     factory));

  const auto* control_type =
      AsTypeDefinition(space.GetNode(data_items::id::DataItemControlType));
  ASSERT_TRUE(control_type);

  const NodeId instance_id{900001, data_items::id::DataItemControlType
                                       .namespace_index()};
  auto [instance_status, instance] = factory.CreateNode(NodeState{
      instance_id, NodeClass::Object, data_items::id::DataItemControlType,
      data_items::id::DataItems, id::Organizes,
      NodeAttributes{.browse_name = QualifiedName{"TestControl"}}});
  ASSERT_TRUE(instance_status);

  ASSERT_TRUE(CreateMissingChildren(factory, instance_id, *control_type));

  for (std::string_view name : {"Select", "Operate", "Cancel"}) {
    const Node* method = space.GetNode(MakeNestedNodeId(instance_id, name));
    ASSERT_TRUE(method) << name << " was not materialized";
    EXPECT_EQ(method->GetNodeClass(), NodeClass::Method);
    // A Method node has no type definition, and must not be given one.
    EXPECT_FALSE(method->type_definition());
  }
}

// An Object component recurses: instantiating DataItemType with components
// enabled must produce Control AND the three methods underneath it.
TEST(NodeFactoryUtil, ObjectComponentsRecurseIntoTheirOwnChildren) {
  AddressSpaceImpl2 space;
  GenericNodeFactory loader{space};
  ASSERT_TRUE(LoadStaticAddressSpace(GetScadaStaticNodesetSourcePaths(), space,
                                     loader));

  const auto* data_item_type =
      AsTypeDefinition(space.GetNode(data_items::id::DataItemType));
  ASSERT_TRUE(data_item_type);

  GenericNodeFactory factory{space, /*create_properties=*/true,
                             /*create_components=*/true};
  const NodeId instance_id{900002,
                           data_items::id::DataItemType.namespace_index()};
  auto [instance_status, instance] = factory.CreateNode(
      NodeState{instance_id, NodeClass::Variable, data_items::id::DataItemType,
                data_items::id::DataItems, id::Organizes,
                NodeAttributes{.browse_name = QualifiedName{"TestItem"}}});
  ASSERT_TRUE(instance_status);

  const NodeId control_id = MakeNestedNodeId(instance_id, "Control");
  ASSERT_TRUE(space.GetNode(control_id)) << "Control was not materialized";
  EXPECT_TRUE(space.GetNode(MakeNestedNodeId(control_id, "Select")))
      << "recursion stopped before the Control object's own methods";
}

}  // namespace
}  // namespace scada
