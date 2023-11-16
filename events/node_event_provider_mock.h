#pragma once

#include "events/node_event_provider.h"

#include <gmock/gmock.h>

class MockNodeEventProvider : public NodeEventProvider {
 public:
  MockNodeEventProvider() {
    ON_CALL(*this, unacked_events())
        .WillByDefault(testing::ReturnRef(unacked_events_));
  }

  MOCK_METHOD(scada::EventSeverity, severity_min, (), (const override));
  MOCK_METHOD(void,
              SetSeverityMin,
              (scada::EventSeverity severity),
              (override));

  MOCK_METHOD(const EventContainer&, unacked_events, (), (const override));

  MOCK_METHOD(const EventSet*,
              GetItemUnackedEvents,
              (const scada::NodeId& node_id),
              (const override));

  MOCK_METHOD(void, AcknowledgeEvent, (scada::EventId ack_id), (override));
  MOCK_METHOD(void,
              AcknowledgeItemEvents,
              (const scada::NodeId& node_id),
              (override));
  MOCK_METHOD(void, AcknowledgeAllEvents, (), (override));

  MOCK_METHOD(bool, IsAcking, (), (const override));
  MOCK_METHOD(bool,
              IsAlerting,
              (const scada::NodeId& node_id),
              (const override));

  MOCK_METHOD(void, AddObserver, (EventObserver & observer), (override));
  MOCK_METHOD(void, RemoveObserver, (EventObserver & observer), (override));

  MOCK_METHOD(void,
              AddItemObserver,
              (const scada::NodeId& node_id, EventObserver& observer),
              (override));
  MOCK_METHOD(void,
              RemoveItemObserver,
              (const scada::NodeId& node_id, EventObserver& observer),
              (override));

  EventContainer unacked_events_;
};
