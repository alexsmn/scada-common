#pragma once

#include "core/monitored_item.h"
#include "core/monitored_item_service.h"
#include "core/view_events.h"

class IViewEventsSubscription {
 public:
  virtual ~IViewEventsSubscription() = default;
};

// TODO: Extract to a separate file.
using ViewEventsProvider =
    std::function<std::unique_ptr<IViewEventsSubscription>(
        scada::ViewEvents& events)>;

inline std::shared_ptr<scada::MonitoredItem>
CreateModelChangeEventsMonitoredItem(
    scada::MonitoredItemService& monitored_item_service) {
  return monitored_item_service.CreateMonitoredItem(
      scada::ReadValueId{scada::id::Server, scada::AttributeId::EventNotifier},
      scada::MonitoringParameters{}.set_filter(scada::EventFilter{}.set_of_type(
          {scada::id::GeneralModelChangeEventType})));
}

inline std::shared_ptr<scada::MonitoredItem>
CreateModelAndSemanticChangeEventsMonitoredItem(
    scada::MonitoredItemService& monitored_item_service) {
  return monitored_item_service.CreateMonitoredItem(
      scada::ReadValueId{scada::id::Server, scada::AttributeId::EventNotifier},
      scada::MonitoringParameters{}.set_filter(scada::EventFilter{}.set_of_type(
          {scada::id::GeneralModelChangeEventType,
           scada::id::SemanticChangeEventType})));
}

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
      monitored_item_{CreateModelAndSemanticChangeEventsMonitoredItem(
          monitored_item_service)} {
  assert(monitored_item_);
  // FIXME: Capturing |this|.
  monitored_item_->Subscribe(
      [this](const scada::Status& status, const std::any& event) {
        assert(status);
        OnEvent(event);
      });
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

inline ViewEventsProvider MakeViewEventsProvider(
    scada::MonitoredItemService& monitored_item_service) {
  return [&monitored_item_service](scada::ViewEvents& events) {
    return std::make_unique<ViewEventsSubscription>(monitored_item_service,
                                                    events);
  };
}
