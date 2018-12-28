#pragma once

#include "base/span.h"
#include "core/data_value.h"
#include "core/node_id.h"

#include <functional>

namespace scada {

struct AggregateFilter;

using Aggregator = std::function<DataValue(span<const DataValue> values)>;

Aggregator GetAggregator(const NodeId& aggregate_type,
                         DateTime start_time,
                         bool forward);

DateTime GetLocalAggregateStartTime();

DateTime GetAggregateIntervalStartTime(DateTime time,
                                       DateTime start_time,
                                       Duration interval);

struct AggregateState {
  void Process(span<const scada::DataValue> raw_span);
  void Finish();

  bool forward = true;
  const scada::AggregateFilter& aggregation;
  std::vector<scada::DataValue>& data_values;

  scada::Aggregator aggregator;
  scada::DateTime aggregator_start_time;
  scada::DataValue aggregated_value;
};

}  // namespace scada
