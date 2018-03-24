#pragma once

#include "core/node_id.h"

namespace scada {

struct ModelChangeEvent {
  enum Verb : uint8_t {
    NodeAdded = 1 << 0,
    NodeDeleted = 1 << 1,
    ReferenceAdded = 1 << 2,
    ReferenceDeleted = 1 << 3,
    DataTypeChanged = 1 << 4,
  };

  scada::NodeId node_id;
  scada::NodeId type_definition_id;
  uint8_t verb;
};

}  // namespace scada
