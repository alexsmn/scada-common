#pragma once

#include "common/node_state.h"

namespace scada {

// Returns empty variant on error.
inline Variant Read(const NodeState& node_state, AttributeId attribute_id) {
  switch (attribute_id) {
    case AttributeId::NodeId:
      return node_state.node_id;
    case AttributeId::NodeClass:
      return static_cast<scada::Int32>(node_state.node_class);
    case AttributeId::BrowseName:
      return node_state.attributes.browse_name;
    case AttributeId::DisplayName:
      return node_state.attributes.display_name;
    case AttributeId::DataType:
      return node_state.attributes.data_type;
    case AttributeId::Value:
      return node_state.attributes.value.value_or(scada::Variant{});
    default:
      return {};
  }
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
