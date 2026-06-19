#include "address_space/address_space_impl3.h"

#include "address_space/address_space_xml.h"
#include "address_space/generic_node_factory.h"
#include "model/static_nodesets.h"

AddressSpaceImpl3::AddressSpaceImpl3() {
  GenericNodeFactory node_factory{*this};
  [[maybe_unused]] const auto status = scada::LoadStaticAddressSpace(
      scada::GetScadaStaticNodesetSourcePaths(), *this, node_factory);
  assert(status);
}
