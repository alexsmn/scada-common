#pragma once

#include "address_space/address_space_impl.h"

struct StandardAddressSpace;
struct StaticAddressSpace;

class AddressSpaceImpl2 : public AddressSpaceImpl {
 public:
  explicit AddressSpaceImpl2(std::shared_ptr<Logger> logger);
  ~AddressSpaceImpl2();

 private:
  std::unique_ptr<StandardAddressSpace> standard_address_space_;
  std::unique_ptr<StaticAddressSpace> static_address_space_;
};
