#pragma once

#include "base/awaitable.h"
#include "scada/event.h"
#include "scada/status_or.h"

// Events are delivered following the `EventNotifier` hierarchy starting from
// the event node.
class EventNotifier {
 public:
  virtual ~EventNotifier() {}

  // TODO: Introduce batch interface. See `EventProducer` for reasoning.
  virtual void NotifyEvent(const scada::Event& event) = 0;

  [[nodiscard]] virtual Awaitable<scada::StatusOr<scada::EventId>>
  NotifyEventAsync(scada::Event event) = 0;

  virtual void NotifyModelChanged(const scada::ModelChangeEvent& event) = 0;

  virtual void NotifySemanticChanged(
      const scada::SemanticChangeEvent& event) = 0;
};
