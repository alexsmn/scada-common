#pragma once

#include "core/configuration_types.h"
#include "core/event.h"
#include "core/history_types.h"

namespace scada {

struct Aggregation;

class HistoryService {
 public:
  virtual ~HistoryService() {}

  virtual void HistoryReadRaw(const NodeId& node_id,
                              base::Time from,
                              base::Time to,
                              const Aggregation& aggregation,
                              const HistoryReadRawCallback& callback) = 0;

  virtual void HistoryReadEvents(const NodeId& node_id,
                                 base::Time from,
                                 base::Time to,
                                 const EventFilter& filter,
                                 const HistoryReadEventsCallback& callback) = 0;
};

}  // namespace scada
