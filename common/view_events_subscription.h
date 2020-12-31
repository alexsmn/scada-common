#pragma once

#include "core/monitored_item.h"
#include "core/monitored_item_service.h"
#include "core/view_events.h"

class IViewEventsSubscription {
 public:
  virtual ~IViewEventsSubscription() = default;
};

class ViewEventsSubscription : public IViewEventsSubscription {
 public:
  explicit ViewEventsSubscription(
      scada::MonitoredItemService& monitored_item_service,
      scada::ViewEvents& events);

 private:
  void OnEvent(const std::any& event);

  scada::ViewEvents& events_;
  const std::shared_ptr<scada::MonitoredItem> monitored_item_;
};

inline ViewEventsSubscription::ViewEventsSubscription(
    scada::MonitoredItemService& monitored_item_service,
    scada::ViewEvents& events)
    : events_{events},
      monitored_item_{monitored_item_service.CreateMonitoredItem(
          {scada::id::Server, scada::AttributeId::EventNotifier},
          {scada::EventFilter{{},
                              {scada::id::GeneralModelChangeEventType,
                               scada::id::SemanticChangeEventType}}})} {
  assert(monitored_item_);
  monitored_item_->set_event_handler(
      [this](const scada::Status& status, const std::any& event) {
        assert(status);
        OnEvent(event);
      });
  monitored_item_->Subscribe();
}

inline void ViewEventsSubscription::OnEvent(const std::any& event) {
  if (auto* model_change_event =
          std::any_cast<scada::ModelChangeEvent>(&event)) {
    events_.OnModelChanged(*model_change_event);
  } else if (auto* semantic_change_event =
                 std::any_cast<scada::SemanticChangeEvent>(&event)) {
    events_.OnNodeSemanticsChanged(*semantic_change_event);
  } else {
    assert(false);
  }
}