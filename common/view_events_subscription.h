#pragma once

#include "core/view_service.h"

class ViewEventsSubscription {
 public:
  explicit ViewEventsSubscription(scada::ViewService& service,
                                  scada::ViewEvents& events);
  ~ViewEventsSubscription();

 private:
  scada::ViewService& service_;
  scada::ViewEvents& events_;
};

inline ViewEventsSubscription::ViewEventsSubscription(
    scada::ViewService& service,
    scada::ViewEvents& events)
    : service_{service}, events_{events} {
  service_.Subscribe(events_);
}

inline ViewEventsSubscription::~ViewEventsSubscription() {
  service_.Unsubscribe(events_);
}
