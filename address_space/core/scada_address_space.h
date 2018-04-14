#pragma once

#include "common/scada_node_ids.h"
#include "core/configuration_types.h"
#include "core/standard_address_space.h"

class ConfigurationImpl;
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

  scada::ReferenceType Creates{id::Creates, "Creates", L"Creates"};

  DeviceType DeviceType;
};

void CreateScadaAddressSpace(ConfigurationImpl& configuration,
                             NodeFactory& node_factory);
