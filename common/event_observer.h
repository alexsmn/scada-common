#pragma once

#include "core/event.h"

class EventSet;

class EventObserver {
 public:
  virtual ~EventObserver() = default;

  virtual void OnEventReported(const scada::Event& event) {}
  virtual void OnEventAcknowledged(const scada::Event& event) {}
  virtual void OnAllEventsAcknowledged() {}
  virtual void OnItemEventsChanged(const scada::NodeId& item_id,
                                   const EventSet& events) {}
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) {}
  virtual void OnSemanticChanged(const scada::SemanticChangeEvent& event) {}
};
