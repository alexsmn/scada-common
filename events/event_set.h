#pragma once

#include "scada/event.h"

#include <set>

struct EventComparer {
  bool operator()(const scada::Event* left, const scada::Event* right) const {
    if (left->time < right->time)
      return true;
    if (right->time < left->time)
      return false;
    return left < right;
  }
};

class EventSet : public std::set<const scada::Event*, EventComparer> {
};
