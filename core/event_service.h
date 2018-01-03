#pragma once

#include "core/node_id.h"

namespace scada {

class Event;

class EventService {
 public:
  virtual ~EventService() {}

  virtual void Acknowledge(int acknowledge_id, const NodeId& user_node_id) = 0;

  virtual void GenerateEvent(const Event& event) = 0;
};

} // namespace scada