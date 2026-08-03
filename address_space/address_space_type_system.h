#pragma once

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "common/type_system.h"

class AddressSpaceTypeSystem final : public TypeSystem {
 public:
  explicit AddressSpaceTypeSystem(const scada::AddressSpace& address_space)
      : address_space_{address_space} {}

  // TypeSystem
  virtual bool IsSubtypeOf(const scada::NodeId& type_id,
                           const scada::NodeId& supertype_id) const override {
    return scada::IsSubtypeOf(address_space_, type_id, supertype_id);
  }

  const scada::AddressSpace& address_space_;
};
