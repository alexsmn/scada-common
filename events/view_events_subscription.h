#pragma once

#include "base/any_executor.h"
#include "base/check.h"
#include "scada/legacy_monitored_item_adapter.h"
#include "scada/monitored_item.h"
#include "scada/monitored_item_service.h"
#include "scada/monitoring_parameters.h"
#include "scada/read_value_id.h"
#include "scada/standard_node_ids.h"
#include "scada/status.h"
#include "scada/view_events.h"

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
    scada::LegacyMonitoredItemAdapter& adapter) {
  return adapter.CreateMonitoredItem(
      scada::ReadValueId{scada::id::Server, scada::AttributeId::EventNotifier},
      scada::MonitoringParameters{
          .filter = scada::EventFilter{
              .of_type = {scada::id::GeneralModelChangeEventType}}});
}

inline std::shared_ptr<scada::MonitoredItem>
CreateModelAndSemanticChangeEventsMonitoredItem(
    scada::LegacyMonitoredItemAdapter& adapter) {
  return adapter.CreateMonitoredItem(
      scada::ReadValueId{scada::id::Server, scada::AttributeId::EventNotifier},
      scada::MonitoringParameters{
          .filter = scada::EventFilter{
              .of_type = {scada::id::GeneralModelChangeEventType,
                          scada::id::SemanticChangeEventType}}});
}

class ViewEventsSubscription : public IViewEventsSubscription {
 public:
  explicit ViewEventsSubscription(
      AnyExecutor executor,
      scada::MonitoredItemService& monitored_item_service,
      scada::ViewEvents& events);

 private:
  void OnEvent(const std::any& event);

  scada::ViewEvents& events_;
  // Declared before |monitored_item_| so it is constructed first; the adapter
  // owns the subscription that backs the legacy monitored item.
  scada::LegacyMonitoredItemAdapter monitored_item_adapter_;
  const std::shared_ptr<scada::MonitoredItem> monitored_item_;
};

inline ViewEventsSubscription::ViewEventsSubscription(
    AnyExecutor executor,
    scada::MonitoredItemService& monitored_item_service,
    scada::ViewEvents& events)
    : events_{events},
      monitored_item_adapter_{executor, monitored_item_service},
      monitored_item_{CreateModelAndSemanticChangeEventsMonitoredItem(
          monitored_item_adapter_)} {
  base::Check(monitored_item_);
  // FIXME: Capturing |this|.
  monitored_item_->Subscribe(
      [this](const scada::Status& status, const std::any& event) {
        // A bad status may be reported for a remote subscription; skip it.
        if (status && event.has_value()) {
          OnEvent(event);
        }
      });
}

inline void ViewEventsSubscription::OnEvent(const std::any& event) {
  if (auto* model_change_event =
          std::any_cast<scada::ModelChangeEvent>(&event)) {
    events_.OnModelChanged(*model_change_event);
  } else if (auto* semantic_change_event =
                 std::any_cast<scada::SemanticChangeEvent>(&event)) {
    events_.OnNodeSemanticsChanged(*semantic_change_event);
  }
  // Events arrive from a (possibly remote) server; unknown event types are
  // ignored.
}

inline ViewEventsProvider MakeViewEventsProvider(
    AnyExecutor executor,
    scada::MonitoredItemService& monitored_item_service) {
  return [executor, &monitored_item_service](scada::ViewEvents& events) {
    return std::make_unique<ViewEventsSubscription>(
        executor, monitored_item_service, events);
  };
}
