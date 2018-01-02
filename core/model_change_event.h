#pragma once

#include <cstdint>

namespace scada {

enum ModelChangeEventVerb : uint8_t {
    NodeAdded        = 1 << 0,
    NodeDeleted      = 1 << 1,
    ReferenceAdded   = 1 << 2,
    ReferenceDeleted = 1 << 3,
    DataTypeChanged  = 1 << 4,
};

} // namespace scada
