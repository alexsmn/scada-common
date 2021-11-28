#include "address_space/address_space_impl3.h"

#include "address_space/generic_node_factory.h"
#include "address_space/scada_address_space.h"

AddressSpaceImpl3::AddressSpaceImpl3() {
  GenericNodeFactory node_factory{*this};
  ScadaAddressSpaceBuilder{*this, node_factory}.BuildAll();
}
