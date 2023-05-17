#pragma once

#include "address_space/address_space_impl2.h"
#include "address_space/address_space_type_system.h"

class StandardTypeSystem : public TypeSystem {
 public:
  virtual bool IsSubtypeOf(const scada::NodeId& type_definition_id,
                           const scada::NodeId& supertype_id) const override {
    return type_system_.IsSubtypeOf(type_definition_id, supertype_id);
  }

 private:
  const AddressSpaceImpl2 standard_address_space_;
  const AddressSpaceTypeSystem type_system_{standard_address_space_};
};
