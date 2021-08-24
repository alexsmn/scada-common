#pragma once

namespace scada {
class NodeId;
class Event;
struct ModelChangeEvent;
struct SemanticChangeEvent;
}  // namespace scada

class EventSet;

class EventObserver {
 public:
  virtual ~EventObserver() = default;

  virtual void OnEvent(const scada::Event& event) {}
  virtual void OnAllEventsAcknowledged() {}
  virtual void OnItemEventsChanged(const scada::NodeId& item_id,
                                   const EventSet& events) {}
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) {}
  virtual void OnSemanticChanged(const scada::SemanticChangeEvent& event) {}
};
