#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "core/data_value.h"
#include "core/event.h"

namespace scada {

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
    std::function<void(Status&&, std::vector<DataValue>&& values)>;

using HistoryReadEventsCallback =
    std::function<void(Status&& status, std::vector<Event>&& events)>;

}  // namespace scada
