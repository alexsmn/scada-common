#pragma once

#include "base/span.h"
#include "core/data_value.h"
#include "core/node_id.h"

#include <functional>

namespace scada {

struct AggregateFilter;

using Aggregator = std::function<DataValue(span<const DataValue> values)>;

Aggregator GetAggregator(const NodeId& aggregate_type, DateTime start_time);

DateTime GetLocalAggregateStartTime();

DateTime GetAggregateIntervalStartTime(DateTime time,
                                       DateTime start_time,
                                       Duration interval);

std::vector<DataValue> AggregateRange(span<const DataValue> values,
                                      const AggregateFilter& aggregation);

struct AggregateState {
  void Process(span<const scada::DataValue> raw_span);
  void Finish();

  const scada::AggregateFilter& aggregation;
  std::vector<scada::DataValue>& data_values;

  scada::Aggregator aggregator;
  scada::DateTime aggregator_start_time;
  scada::DataValue aggregated_value;
};

}  // namespace scada
