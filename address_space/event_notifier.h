#pragma once

class EventNotifier {
 public:
  virtual ~EventNotifier() {}

  virtual void NotifyModelChanged(const scada::ModelChangeEvent& event) = 0;

  virtual void NotifySemanticChanged(const scada::NodeId& node_id,
                                     const scada::NodeId& type_definion_id) = 0;
};
