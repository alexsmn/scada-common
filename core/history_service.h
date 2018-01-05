#pragma once

#include "core/configuration_types.h"
#include "core/event.h"
#include "core/history_types.h"

namespace scada {

class HistoryService {
 public:
  virtual ~HistoryService() {}

  virtual void HistoryRead(const ReadValueId& read_value_id, base::Time from, base::Time to,
                           const Filter& filter, const HistoryReadCallback& callback) = 0;
};

} // namespace scada