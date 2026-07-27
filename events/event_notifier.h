#pragma once

#include "base/awaitable.h"
#include "scada/co_result.h"
#include "scada/event.h"
#include "scada/status_or.h"

// Events are delivered following the `EventNotifier` hierarchy starting from
// the event node.
class EventNotifier {
 public:
  virtual ~EventNotifier() {}

  // TODO: Introduce batch interface. See `EventProducer` for reasoning.
  virtual void NotifyEvent(const scada::Event& event) = 0;

  [[nodiscard]] virtual scada::CoStatusOr<scada::EventId> NotifyEventAsync(
      scada::Event event) = 0;

  // Routes an event that another server already produced: unlike NotifyEvent
  // it must carry its origin-assigned event id and receive time, which are
  // preserved so acknowledgement and history keep referring to the same
  // event across tiers. Used by the aggregation proxy to re-raise downstream
  // events upstream (ADR 0004); per-tier event-id bits keep the merged
  // streams collision-free.
  virtual void NotifyForwardedEvent(const scada::Event& event) = 0;

  // Routes a device protocol frame (DeviceFrameEventType). Defaulted rather
  // than pure so every existing notifier keeps compiling and still delivers the
  // frame's log line; a notifier that can carry the structured fields overrides
  // it. Dropping to the base event loses the decoded frame, not the message.
  virtual void NotifyDeviceFrame(const scada::DeviceFrameEvent& event) {
    NotifyEvent(event.base);
  }

  virtual void NotifyModelChanged(const scada::ModelChangeEvent& event) = 0;

  virtual void NotifySemanticChanged(
      const scada::SemanticChangeEvent& event) = 0;
};
