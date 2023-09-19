#pragma once

#include "scada/event.h"

#include <map>

namespace scada {
class NodeId;
}

class EventObserver;
class EventSet;

class NodeEventProvider {
 public:
  virtual ~NodeEventProvider() = default;

  virtual unsigned severity_min() const = 0;
  virtual void SetSeverityMin(unsigned severity) = 0;

  using EventContainer = std::map<unsigned /*ack_id*/, scada::Event>;
  virtual const EventContainer& unacked_events() const = 0;

  virtual const EventSet* GetItemUnackedEvents(
      const scada::NodeId& item_id) const = 0;

  virtual void AcknowledgeEvent(unsigned ack_id) = 0;
  virtual void AcknowledgeItemEvents(const scada::NodeId& item_id) = 0;
  virtual void AcknowledgeAllEvents() = 0;

  virtual bool IsAcking() const = 0;
  virtual bool IsAlerting(const scada::NodeId& item_id) const = 0;

  virtual void AddObserver(EventObserver& observer) = 0;
  virtual void RemoveObserver(EventObserver& observer) = 0;

  virtual void AddItemObserver(const scada::NodeId& item_id,
                               EventObserver& observer) = 0;
  virtual void RemoveItemObserver(const scada::NodeId& item_id,
                                  EventObserver& observer) = 0;
};
