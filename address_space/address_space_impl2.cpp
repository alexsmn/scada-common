#include "address_space/address_space_impl2.h"

#include "address_space/node_builder_impl.h"
#include "address_space/scada_address_space.h"

AddressSpaceImpl2::AddressSpaceImpl2(std::shared_ptr<Logger> logger)
    : AddressSpaceImpl{std::move(logger)},
      standard_address_space_(std::make_unique<StandardAddressSpace>(*this)),
      static_address_space_(
          std::make_unique<StaticAddressSpace>(*this,
                                               *standard_address_space_)) {}

AddressSpaceImpl2::~AddressSpaceImpl2() {
  Clear();
}
