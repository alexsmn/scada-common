#pragma once

#include <gmock/gmock.h>

MATCHER_P(NodeIs, node_id, "") {
  return arg.node_id == node_id;
}
