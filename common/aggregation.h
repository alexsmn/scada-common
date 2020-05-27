#pragma once

#include "base/containers/span.h"
#include "core/data_value.h"
#include "core/node_id.h"

#include <functional>

namespace scada {

struct AggregateFilter;

using Aggregator = std::function<DataValue(base::span<const DataValue> values)>;

Aggregator GetAggregator(const NodeId& aggregate_type,
                         const DateTimeRange& interval,
                         bool forward);

DateTime GetLocalAggregateStartTime();

DateTimeRange GetAggregateInterval(DateTime time,
                                   DateTime start_time,
                                   Duration interval);

struct AggregateState {
  void Process(base::span<const scada::DataValue> raw_span);
  void Finish();

  bool forward = true;
  const scada::AggregateFilter& aggregation;
  std::vector<scada::DataValue>& data_values;

  scada::Aggregator aggregator;
  scada::DateTimeRange aggregator_interval;
  scada::DataValue aggregated_value;
};

}  // namespace scada
