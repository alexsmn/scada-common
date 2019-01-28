#pragma once

namespace scada {
class NodeId;
class Event;
}

class EventSet;

class EventObserver {
 public:
  virtual void OnEventReported(const scada::Event& event) {}
  virtual void OnEventAcknowledged(const scada::Event& event) {}
  virtual void OnAllEventsAcknowledged() {}
  virtual void OnItemEventsChanged(const scada::NodeId& item_id, const EventSet& events) {}
};
