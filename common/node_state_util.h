#pragma once

#include "common/node_state.h"
#include "scada/attribute_service.h"

#include <optional>
#include <vector>

namespace scada {

inline DataValue ReadAttribute(const NodeState& node_state,
                               AttributeId attribute_id) {
  auto optional_value = node_state.GetAttribute(attribute_id);
  return optional_value.has_value()
             ? MakeReadResult(optional_value.value())
             : MakeReadError(StatusCode::Bad_WrongAttributeId);
}

void SortNodesHierarchically(std::vector<NodeState>& nodes);

}  // namespace scada
