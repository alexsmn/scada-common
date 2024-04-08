#pragma once

#include "events/event_notifier.h"

#include <gmock/gmock.h>

class MockEventNotifier : public EventNotifier {
 public:
  MOCK_METHOD(promise<scada::EventId>,
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
