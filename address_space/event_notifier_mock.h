#pragma once

#include "core/model_change_event.h"
#include "core/node_id.h"
#include "event_notifier.h"

#include <gmock/gmock.h>

class MockEventNotifier : public EventNotifier {
 public:
  MOCK_METHOD1(NotifyModelChanged, void(const scada::ModelChangeEvent& event));

  MOCK_METHOD2(NotifySemanticChanged,
               void(const scada::NodeId& node_id,
                    const scada::NodeId& type_definion_id));
};
