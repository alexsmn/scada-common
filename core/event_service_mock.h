#pragma once

#include "core/event.h"
#include "core/event_service.h"

#include <gmock/gmock.h>

namespace scada {

class MockEventService : public EventService {
 public:
  MOCK_METHOD2(Acknowledge,
               void(int acknowledge_id, const NodeId& user_node_id));
  MOCK_METHOD1(GenerateEvent, void(const Event& event));
};

}  // namespace scada
