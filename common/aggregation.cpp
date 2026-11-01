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
// as both sections require.
//
// Both aggregates use Simple Bounding Values (Part 13 §3.1.9 simple bounding
// values): with stepped interpolation the start bound is the first raw value
// before the interval start, whatever its quality. The caller feeds it like
// any other sample; its timestamp is clamped to the interval start, so it
// covers the time up to the first sample inside. A Bad bound covers nothing.
// Without one the time before the first sample is not covered — the "No Start
// Bound" case of Tables 55 and 57, where the bound "is Bad_NoData and treated
// as any other Bad value in the interval".
// https://reference.opcfoundation.org/Core/Part13/v105/docs/3.1.9
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

bool UsesStartBound(const NodeId& aggregate_type) {
  // The aggregates computed by SteppedIntegral. Part 13 Tables 55-57 give
  // them "Use Bounds: Simple" (TimeAverage2, Total2) and "Interpolated"
  // (Total); with a stepped line both reduce to the last value before the
  // interval start.
  return aggregate_type == id::AggregateFunction_TimeAverage2 ||
         aggregate_type == id::AggregateFunction_Total ||
         aggregate_type == id::AggregateFunction_Total2;
}

Time GetLocalAggregateStartTime() {
  base::Exploded exploded = {2000, 1, 0, 1};
  return base::FromLocalExploded(exploded).value_or(Time{});
}

TimeRange GetAggregateInterval(Time time, Time origin_time, Duration interval) {
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
      if (forward) {
        // The value recorded last holds from the previous interval up to
        // this one.
        const auto& held = last_raw ? last_raw : start_bound;
        if (aggregator)
          AppendHeldIntervals(aggregator_interval.second, interval.first, held);
        else if (!IsNull(fill_from))
          AppendHeldIntervals(
              GetAggregateInterval(fill_from, aggregation.start_time,
                                   aggregation.interval)
                  .first,
              interval.first, held);
        StartInterval(interval, held);
      } else {
        // Read backward, this value is the start bound of the interval read
        // before it, and it holds through every interval in between.
        if (aggregator && uses_start_bound())
          aggregated_value = aggregator(raw_span.subspan(0, 1));
        std::optional<DataValue> held{raw_span[0]};
        if (aggregator)
          AppendHeldIntervals(interval.second, aggregator_interval.first, held);
        StartInterval(interval, std::nullopt);
      }
      if (!aggregator)
        break;
    }

    aggregated_value = aggregator(raw_span.subspan(0, count));
    last_raw = raw_span[count - 1];
    raw_span = raw_span.subspan(count);
  }
}

void AggregateState::Finish() {
  if (forward && !IsNull(fill_until)) {
    const auto& held = last_raw ? last_raw : start_bound;
    if (aggregator) {
      AppendHeldIntervals(aggregator_interval.second, fill_until, held);
    } else if (!IsNull(fill_from)) {
      // Nothing was recorded during the whole read.
      AppendHeldIntervals(
          GetAggregateInterval(fill_from, aggregation.start_time,
                               aggregation.interval)
              .first,
          fill_until, held);
    }
  }
  if (!aggregated_value.is_null())
    data_values.emplace_back(std::move(aggregated_value));
  aggregated_value = {};
}

void AggregateState::StartInterval(const scada::TimeRange& interval,
                                   const std::optional<DataValue>& bound) {
  if (!aggregated_value.is_null()) {
    data_values.emplace_back(std::move(aggregated_value));
    aggregated_value = {};
  }

  aggregator_interval = interval;
  aggregator =
      GetAggregator(aggregation.aggregate_type, aggregator_interval, forward);
  if (aggregator && bound && uses_start_bound())
    aggregated_value = aggregator(std::span{&*bound, 1});
}

void AggregateState::AppendHeldIntervals(scada::Time from,
                                         scada::Time to,
                                         const std::optional<DataValue>& held) {
  // |from| and |to| are interval boundaries; the intervals between them saw
  // no raw value, so |held| is their start bound and their only input.
  if (!held || !uses_start_bound() || aggregation.interval <= Duration::zero())
    return;

  // Flush the interval in progress first, so the output stays in read order.
  if (!aggregated_value.is_null()) {
    data_values.emplace_back(std::move(aggregated_value));
    aggregated_value = {};
  }

  const std::span<const DataValue> bound{&*held, 1};
  if (forward) {
    for (Time start = from; start < to && data_values.size() < max_count;
         start += aggregation.interval) {
      TimeRange interval{start, start + aggregation.interval};
      if (auto held_aggregator =
              GetAggregator(aggregation.aggregate_type, interval, forward))
        data_values.emplace_back(held_aggregator(bound));
    }
  } else {
    // Backward: |from| is the earlier boundary, and the output descends.
    for (Time end = to; end > from && data_values.size() < max_count;
         end -= aggregation.interval) {
      TimeRange interval{end - aggregation.interval, end};
      if (auto held_aggregator =
              GetAggregator(aggregation.aggregate_type, interval, forward))
        data_values.emplace_back(held_aggregator(bound));
    }
  }
}

bool AggregateState::uses_start_bound() const {
  return UsesStartBound(aggregation.aggregate_type);
}

}  // namespace scada
