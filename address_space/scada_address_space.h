#pragma once

#include "address_space/standard_address_space.h"
#include "core/configuration_types.h"
#include "model/scada_node_ids.h"

class AddressSpaceImpl;
class NodeFactory;

void CreateScadaAddressSpace(AddressSpaceImpl& address_space,
                             NodeFactory& node_factory);
