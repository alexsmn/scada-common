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

inline bool operator==(const ModelChangeEvent& a, const ModelChangeEvent& b) {
  return a.node_id == b.node_id &&
         a.type_definition_id == b.type_definition_id && a.verb == b.verb;
}

inline std::ostream& operator<<(std::ostream& stream,
                                const ModelChangeEvent& e) {
  return stream << "{" << e.node_id << ", " << e.type_definition_id << ", "
                << static_cast<unsigned>(e.verb) << "}";
}

}  // namespace scada
