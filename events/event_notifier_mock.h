#pragma once

#include "events/event_notifier.h"

#include <gmock/gmock.h>

class MockEventNotifier : public EventNotifier {
 public:
  MOCK_METHOD(scada::status_promise<scada::EventId>,
              NotifyEvent,
              (const scada::Event& event),
              (override));

  MOCK_METHOD(void,
              NotifyModelChanged,
              (const scada::ModelChangeEvent& event),
              (override));

  MOCK_METHOD(void,
              NotifySemanticChanged,
              (const scada::SemanticChangeEvent& event),
              (override));
};
