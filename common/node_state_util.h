#pragma once

#include "common/node_state.h"
#include "core/attribute_service.h"

#include <optional>

namespace scada {

inline scada::DataValue ReadAttribute(const NodeState& node_state,
                                      AttributeId attribute_id) {
  auto optional_value = node_state.GetAttribute(attribute_id);
  return optional_value.has_value()
             ? scada::MakeReadResult(optional_value.value())
             : scada::MakeReadError(scada::StatusCode::Bad_WrongAttributeId);
}

/*inline std::vector<scada::ReferenceDescription> Browse(
    const TypeSystem& type_system,
    const NodeState& node_state,
    const scada::NodeId& reference_type_id,
    bool forward) {
  auto ref1 = Map(node_state.properties, [&](const scada::NodeProperty& prop) {
    return scada::ReferenceDescription{
        scada::id::HasProperty, true,
        MakeNestedNodeId(node_state.node_id, prop.first)};
  });

  auto ref2 = Filter(node_state.references,
                     [&](const scada::ReferenceDescription& ref) {
                       return ref.forward == forward &&
                              type_system.IsSubtypeOf(ref.reference_type_id,
                                                      reference_type_id);
                     });

  return Join({ref1, ref2});
}*/

}  // namespace scada
