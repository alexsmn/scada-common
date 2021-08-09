#pragma once

#include "address_space/address_space_impl.h"

struct StandardAddressSpace;

class AddressSpaceImpl2 : public AddressSpaceImpl {
 public:
  AddressSpaceImpl2();
  ~AddressSpaceImpl2();

 private:
  std::unique_ptr<StandardAddressSpace> standard_address_space_;
};
