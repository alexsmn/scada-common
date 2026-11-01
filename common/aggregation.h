#pragma once

#include "scada/data_value.h"
#include "scada/date_time_range.h"
#include "scada/node_id.h"

#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace scada {

struct AggregateFilter;

using Aggregator = std::function<DataValue(std::span<const DataValue> values)>;

Aggregator GetAggregator(const NodeId& aggregate_type,
                         const TimeRange& interval,
                         bool forward);

// Whether |aggregate_type| also needs the value in force when an interval
// starts — its start bound, the last raw value recorded before the interval.
// True for the time-weighted aggregates (TimeAverage2, Total, Total2). Feed
// the bound to the interval's aggregator before the interval's own values, in
// either direction: the aggregator clamps it to the interval start.
bool UsesStartBound(const NodeId& aggregate_type);

Time GetLocalAggregateStartTime();

TimeRange GetAggregateInterval(Time time, Time start_time, Duration interval);

// Splits raw values, read in one direction, into aggregation intervals and
// appends one aggregated value per interval to |data_values|.
//
// For an aggregate that UsesStartBound(), the value recorded last before an
// interval holds into it, and an interval in which nothing was recorded is
// still produced from the value that held through it.
struct AggregateState {
  // Aggregates the next raw values, in read order.
  void Process(std::span<const scada::DataValue> raw_span);
  // Ends the read and appends the last interval(s).
  void Finish();

  bool forward = true;
  const scada::AggregateFilter& aggregation;
  std::vector<scada::DataValue>& data_values;

  // Forward reads only. |start_bound| is the last raw value before
  // |fill_from|, the start of the first interval of the read; it holds until
  // the first raw value. Finish() extends the value recorded last to
  // |fill_until|. Either time may stay null, which produces nothing beyond
  // the intervals the raw values fall in.
  std::optional<scada::DataValue> start_bound;
  scada::Time fill_from = scada::kNullTime;
  scada::Time fill_until = scada::kNullTime;

  // Intervals in which nothing was recorded stop being produced once
  // |data_values| holds this many, so a long quiet stretch cannot grow it
  // without limit. The caller pages from the last value it returns.
  size_t max_count = std::numeric_limits<size_t>::max();

  scada::Aggregator aggregator;
  scada::TimeRange aggregator_interval;
  scada::DataValue aggregated_value;

  // The raw value processed last, in read order.
  std::optional<scada::DataValue> last_raw;

 private:
  void StartInterval(const scada::TimeRange& interval,
                     const std::optional<scada::DataValue>& bound);
  void AppendHeldIntervals(scada::Time from,
                           scada::Time to,
                           const std::optional<scada::DataValue>& held);
  bool uses_start_bound() const;
};

}  // namespace scada
