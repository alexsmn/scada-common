#pragma once

#include "scada/event.h"
#include <span>

namespace scada {
class NodeId;
struct Event;
struct ModelChangeEvent;
struct SemanticChangeEvent;
}  // namespace scada

class EventSet;

class EventObserver {
 public:
  virtual ~EventObserver() = default;

  virtual void OnEvents(std::span<const scada::Event* const> events) {}
  virtual void OnAllEventsAcknowledged() {}
  virtual void OnItemEventsChanged(const scada::NodeId& item_id,
                                   const EventSet& events) {}
  // A device protocol frame. Defaulted to delivering just the base event, so an
  // observer that does not understand frames still receives the log line — the
  // same degradation the notifier and the sink use. Override to keep the
  // decoded fields.
  virtual void OnDeviceFrame(const scada::DeviceFrameEvent& event) {
    const scada::Event* base = &event.base;
    OnEvents({&base, 1});
  }

  virtual void OnModelChanged(const scada::ModelChangeEvent& event) {}
  virtual void OnSemanticChanged(const scada::SemanticChangeEvent& event) {}
};
