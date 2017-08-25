#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "core/data_value.h"
#include "core/event.h"

namespace scada {

struct EventFilter {
  unsigned types;
};

struct Filter {
  EventFilter event_filter;
};

struct ItemInfo {
  NodeId item;
  DataValue tvq;
  base::Time change_time;
};

typedef std::vector<ItemInfo> ItemInfoVector;

typedef std::vector<DataValue> DataValueVector;

typedef std::vector<Event> EventVector;

typedef std::shared_ptr<ItemInfoVector> ItemInfosResults;
typedef std::function<void(ItemInfosResults)> ItemInfosCallback;

typedef std::shared_ptr<DataValueVector> QueryValuesResults;
typedef std::shared_ptr<EventVector> QueryEventsResults;
typedef std::function<void(Status, QueryValuesResults, QueryEventsResults)> HistoryReadCallback;

} // namespace scada
