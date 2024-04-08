#pragma once

#include "scada/event.h"
#include "base/promise.h"

// Events are delivered following the `EventNotifier` hierarchy starting from
// the event node.
class EventNotifier {
 public:
  virtual ~EventNotifier() {}

  // TODO: Introduce batch interface. See `EventProducer` for reasoning.
  virtual promise<scada::EventId> NotifyEvent(
      const scada::Event& event) = 0;

  virtual void NotifyModelChanged(const scada::ModelChangeEvent& event) = 0;

  virtual void NotifySemanticChanged(
      const scada::SemanticChangeEvent& event) = 0;
};
