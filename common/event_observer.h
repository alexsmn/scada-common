#pragma once

#include "memdb/types.h"
#include "core/configuration_types.h"

namespace scada {
class Event;
}

namespace events {

class EventSet;

class EventObserver {
 public:
  virtual void OnEventReported(const scada::Event& event) {}
  virtual void OnEventAcknowledged(const scada::Event& event) {}
  virtual void OnAllEventsAcknowledged() {}
  virtual void OnItemEventsChanged(const scada::NodeId& item_id, const EventSet& events) {}
};

} // namespace events
