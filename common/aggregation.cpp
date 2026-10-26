#include "common/aggregation.h"

#include "base/check.h"
#include "base/time/calendar.h"
#include "common/data_value_traits.h"
#include "common/timed_data_util.h"
#include "scada/aggregate_filter.h"
#include "scada/data_value.h"
#include "scada/standard_node_ids.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <optional>

namespace scada {

namespace {

using AggregatorFactory =
    std::function<Aggregator(const TimeRange& interval, bool forward)>;

// OPC UA Part 13 computes Average, Minimum, Maximum, Count and the
// time-weighted aggregates over Good raw values only (§5.4.3.5 Average,
// §5.4.3.7 TimeAverage2, §5.4.3.10 Minimum, §5.4.3.11 Maximum, §5.4.3.21
// Count), https://reference.opcfoundation.org/Core/Part13/v105/docs/5.4.3
// A sample flagged bad, offline, stale, misconfigured or failed is not Good.
bool IsGoodSample(const DataValue& data_value) {
  return IsGood(data_value.status_code) &&
         data_value.qualifier.general_bad() == 0;
}

// The numeric value of a Good sample, or nothing for a sample an aggregate
// over numbers must skip.
std::optional<Double> GetGoodDouble(const DataValue& data_value) {
  Double double_value = 0;
  if (!IsGoodSample(data_value) || !data_value.value.get(double_value))
    return std::nullopt;
  return double_value;
}

// The result for an interval with no Good value to compute from. Part 13 calls
// this Bad_NoData; it is never a null DataValue, which callers treat as "no
// result yet" (AggregateMonitoredItem checks for it).
DataValue MakeNoDataValue(const TimeRange& interval) {
  DataValue result{Variant{}, Qualifier{Qualifier::BAD}, interval.first,
                   interval.second};
  result.status_code = StatusCode::Bad;
  return result;
}

Double ToSeconds(Duration duration) {
  return std::chrono::duration<Double>(duration).count();
}

// The area under the stepped line through the Good samples of one interval,
// and the length of time it covers. The archive records changes, so a value
// holds until the next recorded sample (or the interval end) — the Stepped
// case of Part 13 §5.4.3.7 TimeAverage2 and §5.4.3.9 Total2,
// https://reference.opcfoundation.org/Core/Part13/v105/docs/5.4.3.7
//
// A Bad sample ends the preceding Good step and its own duration is left out,
// as both sections require. The aggregator sees only the samples inside the
// interval, never the value in force at its start, so the time before the
// first sample is not covered — Part 13's "No Start Bound" treatment, which
// counts it as Bad.
//
// Samples arrive in chunks, ascending when |forward| and descending
// otherwise, so the state is incremental in both directions.
class SteppedIntegral {
 public:
  SteppedIntegral(const TimeRange& interval, bool forward)
      : interval_{interval}, forward_{forward} {}

  void Add(std::span<const DataValue> values) {
    for (const auto& data_value : values) {
      if (forward_)
        AddForward(data_value);
      else
        AddBackward(data_value);
    }
  }

  // Value x seconds over the covered time.
  Double area() const {
    return area_ + (pending_ ? *pending_value_ * PendingSeconds() : 0);
  }

  // Seconds of the interval covered by Good samples.
  Double covered() const {
    return covered_ + (pending_ ? PendingSeconds() : 0);
  }

 private:
  void AddForward(const DataValue& data_value) {
    Time time = std::clamp(data_value.source_timestamp, interval_.first,
                           interval_.second);
    if (pending_ && pending_value_) {
      Double seconds = ToSeconds(time - pending_time_);
      area_ += *pending_value_ * seconds;
      covered_ += seconds;
    }
    pending_ = true;
    pending_time_ = time;
    pending_value_ = GetGoodDouble(data_value);
  }

  void AddBackward(const DataValue& data_value) {
    Time time = std::clamp(data_value.source_timestamp, interval_.first,
                           interval_.second);
    if (auto value = GetGoodDouble(data_value)) {
      Double seconds = ToSeconds(step_end_ - time);
      area_ += *value * seconds;
      covered_ += seconds;
    }
    step_end_ = time;
  }

  Double PendingSeconds() const {
    return pending_value_ ? ToSeconds(interval_.second - pending_time_) : 0;
  }

  const TimeRange interval_;
  const bool forward_;

  Double area_ = 0;
  Double covered_ = 0;

  // Forward: the latest sample, whose step runs on to the interval end until a
  // later one arrives.
  bool pending_ = false;
  Time pending_time_ = kNullTime;
  std::optional<Double> pending_value_;

  // Backward: where the step of the next (earlier) sample ends.
  Time step_end_ = interval_.second;
};

// Keeps the Good sample with the extreme value, for Minimum and Maximum.
template <class Better>
Aggregator MakeExtremeAggregator(const TimeRange& interval) {
  return [interval, best = std::optional<std::pair<Double, Qualifier>>{}](
             std::span<const DataValue> values) mutable {
    for (const auto& data_value : values) {
      auto value = GetGoodDouble(data_value);
      if (value && (!best || Better{}(*value, best->first)))
        best.emplace(*value, data_value.qualifier);
    }
    if (!best)
      return MakeNoDataValue(interval);
    return DataValue{best->first, best->second, interval.first,
                     interval.second};
  };
}

Aggregator MakeFrontAggregator(const TimeRange& interval) {
  return [interval, start_data_value = DataValue{}](
             std::span<const DataValue> values) mutable {
    if (start_data_value.is_null())
      start_data_value = values[0];
    return DataValue{start_data_value.value, start_data_value.qualifier,
                     interval.first, interval.second};
  };
}

Aggregator MakeBackAggregator(const TimeRange& interval) {
  return [interval](std::span<const DataValue> values) {
    const auto& end_data_value = values[values.size() - 1];
    return DataValue{end_data_value.value, end_data_value.qualifier,
                     interval.first, interval.second};
  };
}

std::map<NodeId, AggregatorFactory> BuildAggregatorFactoryMap() {
  // OPC UA Part 4 - Services 1.03
  // 7.17.4. AggregateFilter
  // The ServerTimestamp for each interval shall be the time of the end of the
  // processing interval.

  std::map<NodeId, AggregatorFactory> aggregators;

  // Part 13 §5.4.3.9 Total2: TimeAverage2 x the Good part of the interval in
  // seconds, i.e. the area under the stepped line, in [value units] x s.
  // https://reference.opcfoundation.org/Core/Part13/v105/docs/5.4.3.9
  //
  // Total (§5.4.3.8) is defined the same way over sloped interpolation and
  // interpolated bounds. Neither fits an archive of recorded changes, which is
  // stepped by nature and has no bound inside one interval, so Total answers
  // with Total2's computation. It used to be a plain sum of the recorded
  // values, which had no physical meaning (backlog 870).
  // https://reference.opcfoundation.org/Core/Part13/v105/docs/5.4.3.8
  auto total = [](const TimeRange& interval, bool forward) -> Aggregator {
    return [interval, integral = SteppedIntegral{interval, forward}](
               std::span<const DataValue> values) mutable {
      integral.Add(values);
      if (integral.covered() <= 0)
        return MakeNoDataValue(interval);
      return DataValue{integral.area(), {}, interval.first, interval.second};
    };
  };
  aggregators.emplace(id::AggregateFunction_Total, total);
  aggregators.emplace(id::AggregateFunction_Total2, total);

  // Part 13 §5.4.3.7 TimeAverage2: the area under the stepped line divided by
  // the Good part of the interval.
  // https://reference.opcfoundation.org/Core/Part13/v105/docs/5.4.3.7
  aggregators.emplace(
      id::AggregateFunction_TimeAverage2,
      [](const TimeRange& interval, bool forward) -> Aggregator {
        return [interval, integral = SteppedIntegral{interval, forward}](
                   std::span<const DataValue> values) mutable {
          integral.Add(values);
          if (integral.covered() <= 0)
            return MakeNoDataValue(interval);
          return DataValue{integral.area() / integral.covered(),
                           {},
                           interval.first,
                           interval.second};
        };
      });

  // Part 13 §5.4.3.5 Average: the sum of the Good raw values divided by their
  // number. It is deliberately not time-weighted; TimeAverage2 is.
  // https://reference.opcfoundation.org/Core/Part13/v105/docs/5.4.3.5
  aggregators.emplace(
      id::AggregateFunction_Average,
      [](const TimeRange& interval, bool forward) -> Aggregator {
        return [interval, total = Double{0},
                count = size_t{0}](std::span<const DataValue> values) mutable {
          for (const auto& data_value : values) {
            if (auto value = GetGoodDouble(data_value)) {
              total += *value;
              ++count;
            }
          }
          if (count == 0)
            return MakeNoDataValue(interval);
          return DataValue{total / static_cast<Double>(count),
                           {},
                           interval.first,
                           interval.second};
        };
      });

  // Part 13 §5.4.3.21 Count: the number of Good raw values; zero when there
  // are none.
  // https://reference.opcfoundation.org/Core/Part13/v105/docs/5.4.3.21
  aggregators.emplace(
      id::AggregateFunction_Count,
      [](const TimeRange& interval, bool forward) -> Aggregator {
        return [interval,
                count = size_t{0}](std::span<const DataValue> values) mutable {
          count += std::ranges::count_if(values, IsGoodSample);
          return DataValue{
              static_cast<UInt64>(count), {}, interval.first, interval.second};
        };
      });

  // Part 13 §5.4.3.10 Minimum / §5.4.3.11 Maximum: the extreme Good raw value.
  // https://reference.opcfoundation.org/Core/Part13/v105/docs/5.4.3.10
  aggregators.emplace(
      id::AggregateFunction_Minimum,
      [](const TimeRange& interval, bool forward) {
        return MakeExtremeAggregator<std::less<Double>>(interval);
      });
  aggregators.emplace(
      id::AggregateFunction_Maximum,
      [](const TimeRange& interval, bool forward) {
        return MakeExtremeAggregator<std::greater<Double>>(interval);
      });

  aggregators.emplace(id::AggregateFunction_Start,
                      [](const TimeRange& interval, bool forward) {
                        return forward ? MakeFrontAggregator(interval)
                                       : MakeBackAggregator(interval);
                      });

  aggregators.emplace(id::AggregateFunction_End,
                      [](const TimeRange& interval, bool forward) {
                        return forward ? MakeBackAggregator(interval)
                                       : MakeFrontAggregator(interval);
                      });

  return aggregators;
}

AggregatorFactory GetAggregatorFactory(const NodeId& aggregate_type) {
  const auto kAggregators = BuildAggregatorFactoryMap();
  auto i = kAggregators.find(aggregate_type);
  return i != kAggregators.end() ? i->second : nullptr;
}

}  // namespace

Aggregator GetAggregator(const NodeId& aggregate_type,
                         const TimeRange& interval,
                         bool forward) {
  auto factory = GetAggregatorFactory(aggregate_type);
  return factory ? factory(interval, forward) : nullptr;
}

Time GetLocalAggregateStartTime() {
  base::Exploded exploded = {2000, 1, 0, 1};
  return base::FromLocalExploded(exploded).value_or(Time{});
}

TimeRange GetAggregateInterval(Time time,
                                   Time origin_time,
                                   Duration interval) {
  // The interval comes from request/configuration data and |time| from stored
  // history values (external input); degrade instead of panicking.
  if (interval == Duration::zero())
    return {time, time};
  if (time < origin_time)
    return {origin_time, origin_time + interval};
  auto delta = time - origin_time;
  delta -= delta % interval;
  return {origin_time + delta, origin_time + delta + interval};
}

void AggregateState::Process(std::span<const scada::DataValue> raw_span) {
  while (!raw_span.empty()) {
    auto interval = scada::GetAggregateInterval(raw_span[0].source_timestamp,
                                                aggregation.start_time,
                                                aggregation.interval);

    auto count = forward ? UpperBound(raw_span, interval.second)
                         : ReverseUpperBound(raw_span, interval.first);
    base::Check(count != 0);

    if (aggregator_interval != interval) {
      if (!aggregated_value.is_null()) {
        data_values.emplace_back(std::move(aggregated_value));
        aggregated_value = {};
      }

      aggregator_interval = interval;
      aggregator = GetAggregator(aggregation.aggregate_type,
                                 aggregator_interval, forward);
      if (!aggregator)
        break;
    }

    aggregated_value = aggregator(raw_span.subspan(0, count));
    raw_span = raw_span.subspan(count);
  }
}

void AggregateState::Finish() {
  if (!aggregated_value.is_null())
    data_values.emplace_back(std::move(aggregated_value));
}

}  // namespace scada
