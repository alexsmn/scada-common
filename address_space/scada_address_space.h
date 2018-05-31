#pragma once

#include "address_space/standard_address_space.h"
#include "common/scada_node_ids.h"
#include "core/configuration_types.h"

class AddressSpaceImpl;
class NodeFactory;

struct StaticAddressSpace {
  explicit StaticAddressSpace(StandardAddressSpace& std);

  struct DeviceType : public scada::ObjectType {
    DeviceType(StandardAddressSpace& std,
               scada::NodeId id,
               scada::QualifiedName browse_name,
               scada::LocalizedText display_name);

    GenericProperty Disabled;
    GenericDataVariable Online;
    GenericDataVariable Enabled;
  };

  scada::ReferenceType Creates{id::Creates, "Creates", {}};

  DeviceType DeviceType;
};

void CreateScadaAddressSpace(AddressSpaceImpl& address_space,
                             NodeFactory& node_factory);
