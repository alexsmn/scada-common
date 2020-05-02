#pragma once

#include "address_space/standard_address_space.h"
#include "core/configuration_types.h"
#include "model/scada_node_ids.h"

class AddressSpaceImpl;
class NodeFactory;

struct StaticAddressSpace {
  StaticAddressSpace(AddressSpaceImpl& address_space,
                     StandardAddressSpace& std);

  struct DeviceType : public scada::ObjectType {
    DeviceType(StandardAddressSpace& std,
               scada::NodeId id,
               scada::QualifiedName browse_name,
               scada::LocalizedText display_name);

    GenericProperty Disabled;
    GenericDataVariable Online;
    GenericDataVariable Enabled;
  };

  scada::ReferenceType Creates{scada::id::Creates, "Creates", {}};

  DeviceType DeviceType;
};

void CreateScadaAddressSpace(AddressSpaceImpl& address_space,
                             NodeFactory& node_factory);
