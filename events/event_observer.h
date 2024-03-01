#pragma once

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
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) {}
  virtual void OnSemanticChanged(const scada::SemanticChangeEvent& event) {}
};
