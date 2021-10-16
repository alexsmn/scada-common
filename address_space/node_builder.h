#pragma once

namespace scada {

class NodeId;
class Node;

class NodeBuilder {
 public:
  virtual ~NodeBuilder() {}

  virtual const Node& GetNode(const NodeId& node_id) const = 0;
  virtual Node& GetMutableNode(const NodeId& node_id) = 0;

  virtual void AddReference(const NodeId& reference_type_id,
                            Node& source,
                            Node& target) = 0;
};

}  // namespace scada
