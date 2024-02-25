#pragma once

#include <gmock/gmock.h>

MATCHER_P(NodeStateIs, node_state, "") {
  using namespace testing;

  return ExplainMatchResult(
      FieldsAre(node_state.node_id, node_state.node_class,
                node_state.type_definition_id, node_state.parent_id,
                node_state.reference_type_id, node_state.attributes,
                UnorderedElementsAreArray(node_state.properties),
                UnorderedElementsAreArray(node_state.references),
                // TODO: Handle non-empty children.
                /*children=*/IsEmpty(), node_state.supertype_id),
      arg, result_listener);
}

MATCHER(NodeStateEq, "") {
  using namespace testing;

  return ExplainMatchResult(NodeStateIs(std::get<1>(arg)), std::get<0>(arg),
                            result_listener);
}
