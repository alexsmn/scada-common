#pragma once

#include "events/event_notifier.h"
#include "scada/co_result.h"

#include <gmock/gmock.h>

class MockEventNotifier : public EventNotifier {
 public:
  // An unstubbed `Awaitable<T>` return is gmock's `T()`: a null-frame
  // awaitable whose `await_ready()` still answers false, so `co_await` on it
  // segfaults inside the AWAITING coroutine, naming no mock. Every awaitable
  // method therefore completes by default with the emptiest honest answer.
  // See CLAUDE.md, "Unit Test Guidance"; the `*_mock_unittest.cpp` beside
  // this header pins it.
  MockEventNotifier() {
    ON_CALL(*this, NotifyEventAsync)
        .WillByDefault([](scada::Event) -> scada::CoStatusOr<scada::EventId> {
          co_return scada::StatusCode::Bad_NotSupported;
        });
  }

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
