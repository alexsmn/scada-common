#pragma once

#include "common/event_set.h"
#include "core/node_id.h"

class NodeEventProvider {
 public:
  virtual ~NodeEventProvider() = default;

  virtual const EventSet* GetItemUnackedEvents(
      const scada::NodeId& item_id) const = 0;
};
