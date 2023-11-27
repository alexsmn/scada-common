#pragma once

#include <gmock/gmock.h>

MATCHER_P(ProducedEventIs, event, "") {
  auto new_event = event;
  new_event.receive_time = arg.receive_time;
  return arg == new_event;
}
