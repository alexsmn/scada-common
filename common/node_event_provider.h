#pragma once

#include "common/event_set.h"
#include "core/node_id.h"

class EventObserver;

class NodeEventProvider {
 public:
  virtual ~NodeEventProvider() = default;

  virtual unsigned severity_min() const = 0;
  virtual void SetSeverityMin(unsigned severity) = 0;

  using EventContainer = std::map<unsigned, scada::Event>;
  virtual const EventContainer& unacked_events() const = 0;

  virtual const EventSet* GetItemUnackedEvents(
      const scada::NodeId& item_id) const = 0;

  virtual void AcknowledgeEvent(unsigned ack_id) = 0;

  virtual bool is_acking() const = 0;

  virtual void AddObserver(EventObserver& observer) = 0;
  virtual void RemoveObserver(EventObserver& observer) = 0;
};
