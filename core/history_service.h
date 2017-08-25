#pragma once

#include <memory>
#include <vector>

#include "core/configuration_types.h"
#include "core/event.h"
#include "core/history_types.h"

namespace scada {

class HistoryService {
 public:
  virtual ~HistoryService() {}

  // Values -------------------------------------------------------------------

  virtual void QueryItemInfos(const ItemInfosCallback& callback) = 0;
  virtual void WriteItemInfo(const ItemInfo& info) = 0;

  virtual void HistoryRead(const ReadValueId& read_value_id, base::Time from, base::Time to,
                           const Filter& filter, const HistoryReadCallback& callback) = 0;

  // Events -------------------------------------------------------------------

  virtual void WriteEvent(const Event& event) = 0;

  virtual void AcknowledgeEvent(unsigned ack_id, base::Time time,
                                const NodeId& user_node_id) = 0;
};

} // namespace scada