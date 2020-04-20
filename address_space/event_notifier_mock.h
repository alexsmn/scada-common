#pragma once

#include "core/event.h"
#include "core/node_id.h"
#include "event_notifier.h"

#include <gmock/gmock.h>

class MockEventNotifier : public EventNotifier {
 public:
  MOCK_METHOD1(NotifyEvent, void(const scada::Event& event));

  MOCK_METHOD1(NotifyModelChanged, void(const scada::ModelChangeEvent& event));

  MOCK_METHOD1(NotifySemanticChanged,
               void(const scada::SemanticChangeEvent& event));
};
