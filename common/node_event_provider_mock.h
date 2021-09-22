#pragma once

#include "common/node_event_provider.h"

#include <gmock/gmock.h>

class MockNodeEventProvider : public NodeEventProvider {
 public:
  MOCK_METHOD(const EventSet*,
              GetItemUnackedEvents,
              (const scada::NodeId& item_id),
              (const override));
};
