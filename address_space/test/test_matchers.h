#pragma once

#include "model/node_id_util.h"

#include <gmock/gmock.h>

MATCHER_P(NodeIs, node_id, "") {
  return arg.node_id == node_id;
}

MATCHER_P(NodeIsOrIsNestedOf, node_id, "") {
  if (arg.node_id == node_id)
    return true;

  scada::NodeId parent_id;
  std::string_view nested_name;
  return IsNestedNodeId(arg.node_id, parent_id, nested_name) &&
         parent_id == node_id;
}
