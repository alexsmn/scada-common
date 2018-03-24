#pragma once

namespace scada {

class Node;
class NodeId;
class NodeObserver;

class Configuration {
 public:
  virtual ~Configuration() {}
  
  virtual Node* GetNode(const NodeId& node_id) = 0;

  virtual void Subscribe(NodeObserver& events) const = 0;
  virtual void Unsubscribe(NodeObserver& events) const = 0;

  virtual void SubscribeNode(const NodeId& node_id, NodeObserver& events) const = 0;
  virtual void UnsubscribeNode(const NodeId& node_id, NodeObserver& events) const = 0;
};

using AddressSpace = Configuration;

} // namespace scada
