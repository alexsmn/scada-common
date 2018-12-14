#pragma once

#include "core/date_time.h"
#include "core/node_id.h"

#include <functional>

namespace scada {

class DataValue;

struct AggregateFilter {
  bool is_null() const { return interval.is_zero(); }

  bool operator<(const AggregateFilter& other) const {
    return std::tie(interval, aggregate_type) <
           std::tie(other.interval, other.aggregate_type);
  }

  Duration interval;
  NodeId aggregate_type;
};

std::vector<DataValue> Aggregate(const std::vector<DataValue>& values,
                                 const AggregateFilter& aggregation);

}  // namespace scada
