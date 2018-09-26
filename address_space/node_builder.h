#pragma once

#include "address_space/node_utils.h"

namespace scada {

class NodeId;
class Node;

class NodeBuilder {
 public:
  virtual ~NodeBuilder() {}

  virtual Node& GetNode(const NodeId& node_id) = 0;

  virtual void AddReference(const NodeId& reference_type_id,
                            Node& source,
                            Node& target) = 0;
};

}  // namespace scada
