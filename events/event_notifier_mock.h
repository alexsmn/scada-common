#pragma once

#include "events/event_notifier.h"
#include "scada/co_result.h"

#include <gmock/gmock.h>

class MockEventNotifier : public EventNotifier {
 public:
  MOCK_METHOD(void, NotifyEvent, (const scada::Event& event), (override));
  MOCK_METHOD(void,
              NotifyDeviceFrame,
              (const scada::DeviceFrameEvent& event),
              (override));

  MOCK_METHOD((scada::CoStatusOr<scada::EventId>),
              NotifyEventAsync,
              (scada::Event event),
              (override));

  MOCK_METHOD(void,
              NotifyForwardedEvent,
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
