#pragma once

#include "core/status.h"

namespace scada {
class Node;
struct NodeState;
}  // namespace scada

class NodeFactory {
 public:
  virtual ~NodeFactory() {}

  virtual std::pair<scada::Status, scada::Node*> CreateNode(
      const scada::NodeState& node_state) = 0;
};
