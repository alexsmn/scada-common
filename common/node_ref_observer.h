#pragma once

namespace scada {
class NodeId;
}

class NodeRefObserver {
 public:
  virtual void OnNodeAdded(const scada::NodeId& node_id) {}
  virtual void OnNodeDeleted(const scada::NodeId& node_id) {}
  virtual void OnReferenceAdded(const scada::NodeId& node_id) {}
  virtual void OnReferenceDeleted(const scada::NodeId& node_id) {}
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) {}
};
