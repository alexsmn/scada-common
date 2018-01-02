#pragma once

#include <functional>
#include <vector>

#include "core/configuration_types.h"
#include "core/data_value.h"
#include "core/status.h"

namespace scada {

class Event;
class WriteFlags;

typedef std::function<void(const DataValue& data_value)> DataChangeHandler;
typedef std::function<void(const Status& status, const Event& event)> EventHandler;

class MonitoredItem {
 public:
  MonitoredItem() {}
  virtual ~MonitoredItem() {}

  void set_data_change_handler(DataChangeHandler handler) {
    data_change_handler_ = std::move(handler);
  }
  void set_event_handler(EventHandler handler) {
    event_handler_ = std::move(handler);
  }

  virtual void Subscribe() = 0;

  void ForwardData(const DataValue& data_value);
  void ForwardEvent(const Event& event);

 protected:
  DataChangeHandler data_change_handler_;
  EventHandler event_handler_;

  DISALLOW_COPY_AND_ASSIGN(MonitoredItem);
};

} // namespace scada