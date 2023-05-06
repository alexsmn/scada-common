#pragma once

#include "common/node_event_provider.h"

#include <gmock/gmock.h>

class MockNodeEventProvider : public NodeEventProvider {
 public:
  MOCK_METHOD(unsigned, severity_min, (), (const override));
  MOCK_METHOD(void, SetSeverityMin, (unsigned severity), (override));

  MOCK_METHOD(const EventContainer&, unacked_events, (), (const override));

  MOCK_METHOD(const EventSet*,
              GetItemUnackedEvents,
              (const scada::NodeId& item_id),
              (const override));

  MOCK_METHOD(void, AcknowledgeEvent, (unsigned ack_id), (override));

  MOCK_METHOD(bool, is_acking, (), (const override));

  MOCK_METHOD(void, AddObserver, (EventObserver & observer), (override));
  MOCK_METHOD(void, RemoveObserver, (EventObserver & observer), (override));
};
