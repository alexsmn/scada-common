#pragma once

#include "scada/event.h"
#include "scada/node_id.h"
#include "event_notifier.h"

#include <gmock/gmock.h>

class MockEventNotifier : public EventNotifier {
 public:
  MOCK_METHOD(void, NotifyEvent, (const scada::Event& event), (override));

  MOCK_METHOD(void,
              NotifyModelChanged,
              (const scada::ModelChangeEvent& event),
              (override));

  MOCK_METHOD(void,
              NotifySemanticChanged,
              (const scada::SemanticChangeEvent& event),
              (override));
};
