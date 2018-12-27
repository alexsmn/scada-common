#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "core/aggregate_filter.h"
#include "core/data_value.h"
#include "core/event.h"

namespace scada {

struct HistoryReadRawDetails {
  NodeId node_id;
  base::Time from;
  base::Time to;
  size_t max_count = 0;
  AggregateFilter aggregation;
  bool release_continuation_point = false;
  ByteString continuation_point;
};

struct EventFilter {
  unsigned types = 0;
};

struct ItemInfo {
  NodeId item;
  DataValue tvq;
  base::Time change_time;
};

using ItemInfosCallback =
    std::function<void(std::vector<ItemInfo>&& item_infos)>;

using HistoryReadRawCallback =
    std::function<void(Status&&,
                       std::vector<DataValue>&& values,
                       ByteString&& continuation_point)>;

using HistoryReadEventsCallback =
    std::function<void(Status&& status, std::vector<Event>&& events)>;

}  // namespace scada
