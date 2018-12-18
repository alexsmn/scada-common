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
    return std::tie(start_time, interval, aggregate_type) <
           std::tie(start_time, other.interval, other.aggregate_type);
  }

  DateTime start_time;
  Duration interval;
  NodeId aggregate_type;
};

using Aggregator = std::function<DataValue(span<const DataValue> values)>;

Aggregator GetAggregator(const NodeId& aggregate_type, DateTime start_time);

DateTime GetLocalAggregateStartTime();

DateTime GetAggregateIntervalStartTime(DateTime time,
                                       DateTime start_time,
                                       Duration interval);

std::vector<DataValue> AggregateRange(span<const DataValue> values,
                                      const AggregateFilter& aggregation);

}  // namespace scada
