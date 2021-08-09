#include "address_space/address_space_impl2.h"

#include "address_space/standard_address_space.h"

AddressSpaceImpl2::AddressSpaceImpl2()
    : standard_address_space_(std::make_unique<StandardAddressSpace>(*this)) {}

AddressSpaceImpl2::~AddressSpaceImpl2() {
  Clear();
}
