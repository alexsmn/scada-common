#pragma once

#include "core/date_time.h"
#include "core/node_id.h"

namespace scada {

class DataValue;

struct AggregateFilter {
  bool is_null() const { return interval.is_zero(); }

  bool operator<(const AggregateFilter& other) const {
    return std::tie(start_time, interval, aggregate_type) <
           std::tie(start_time, other.interval, other.aggregate_type);
  }

  DateTime start_time;
  Duration interval;
  NodeId aggregate_type;
};

}  // namespace scada
