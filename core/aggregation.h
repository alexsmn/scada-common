#pragma once

#include "base/span.h"
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

using Aggregator = std::function<DataValue(span<const DataValue> values)>;

Aggregator GetAggregator(const NodeId& aggregate_type, DateTime start_time);

DateTime GetAggregateStartTime(DateTime time, Duration interval);

std::vector<DataValue> AggregateRange(span<const DataValue> values,
                                      const AggregateFilter& aggregation);

}  // namespace scada
