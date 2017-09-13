#pragma once

#include "core/qualifier.h"
#include "core/variant.h"

namespace scada {

class VQ {
 public:
  VQ() {}
  explicit VQ(Variant value, Qualifier qualifier = Qualifier())
      : value(std::move(value)),
        qualifier(qualifier) {
  }
  
  bool is_null() const {
    return value.is_null() && (qualifier.raw() == 0);
  }

  bool operator==(const VQ& other) const {
    return value == other.value && qualifier == other.qualifier;
  }

  bool operator!=(const VQ& other) const { return !operator==(other); }

  Variant value;
  Qualifier qualifier;
};

} // namespace scada
