#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/data_value.h"
#include "core/event.h"

namespace scada {

struct EventFilter {
  unsigned types;
};

struct ItemInfo {
  NodeId item;
  DataValue tvq;
  base::Time change_time;
};

using ItemInfosCallback =
    std::function<void(std::vector<ItemInfo>&& item_infos)>;

using ContinuationPoint = std::string;

using HistoryReadRawCallback =
    std::function<void(Status&&,
                       std::vector<DataValue>&& values,
                       scada::ContinuationPoint&& continuation_point)>;

using HistoryReadEventsCallback =
    std::function<void(Status&& status, std::vector<Event>&& events)>;

}  // namespace scada
