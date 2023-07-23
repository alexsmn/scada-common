#include "common/aggregation.h"

#include "common/data_value_util.h"
#include "scada/aggregate_filter.h"
#include "scada/data_value.h"
#include "scada/standard_node_ids.h"

#include <algorithm>
#include <map>
#include <numeric>

namespace scada {

namespace {

using AggregatorFactory =
    std::function<Aggregator(const DateTimeRange& interval, bool forward)>;

struct CompareVariants {
  bool operator()(const Variant& left, const Variant& right) const {
    Double left_double = 0;
    Double right_double = 0;
    if (left.get(left_double) && right.get(right_double))
      return left_double < right_double;
    else
      return &left < &right;
  }
};

struct CompareDataValues {
  bool operator()(const DataValue& left, const DataValue& right) {
    return CompareVariants{}(left.value, right.value);
  }
};

Double CalculateTotal(base::span<const DataValue> values) {
  return std::accumulate(values.begin(), values.end(), 0.0,
                         [](Double prev_sum, const DataValue& data_value) {
                           Double double_value = 0;
                           if (data_value.value.get(double_value))
                             return prev_sum + double_value;
                           else
                             return prev_sum;
                         });
}

Aggregator MakeFrontAggregator(const DateTimeRange& interval) {
  return [interval, start_data_value = DataValue{}](
             base::span<const DataValue> values) mutable {
    if (start_data_value.is_null())
      start_data_value = values[0];
    return DataValue{start_data_value.value, start_data_value.qualifier,
                     interval.first, interval.second};
  };
}

Aggregator MakeBackAggregator(const DateTimeRange& interval) {
  return [interval](base::span<const DataValue> values) {
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

  aggregators.emplace(
      id::AggregateFunction_Total,
      [](const DateTimeRange& interval, bool forward) {
        Double total = 0;
        return [interval, total](base::span<const DataValue> values) mutable {
          total += CalculateTotal(values);
          return DataValue{total, {}, interval.first, interval.second};
        };
      });

  aggregators.emplace(
      id::AggregateFunction_Average,
      [](const DateTimeRange& interval, bool forward) {
        Double total = 0;
        size_t count = 0;
        return [interval, total,
                count](base::span<const DataValue> values) mutable {
          total += CalculateTotal(values);
          count += values.size();
          assert(count != 0);
          return DataValue{total / count, {}, interval.first, interval.second};
        };
      });

  aggregators.emplace(
      id::AggregateFunction_Count,
      [](const DateTimeRange& interval, bool forward) {
        size_t count = 0;
        return [interval, count](base::span<const DataValue> values) mutable {
          count += values.size();
          return DataValue{count, {}, interval.first, interval.second};
        };
      });

  aggregators.emplace(
      id::AggregateFunction_Minimum,
      [](const DateTimeRange& interval, bool forward) {
        auto min_value = std::numeric_limits<Double>::max();
        return
            [interval, min_value](base::span<const DataValue> values) mutable {
              auto& data_value = *std::min_element(values.begin(), values.end(),
                                                   CompareDataValues{});
              Double double_value = 0;
              if (data_value.value.get(double_value))
                min_value = std::min(min_value, double_value);
              return DataValue{min_value, data_value.qualifier, interval.first,
                               interval.second};
            };
      });

  aggregators.emplace(
      id::AggregateFunction_Maximum,
      [](const DateTimeRange& interval, bool forward) {
        auto max_value = std::numeric_limits<Double>::min();
        return
            [interval, max_value](base::span<const DataValue> values) mutable {
              auto& data_value = *std::max_element(values.begin(), values.end(),
                                                   CompareDataValues{});
              Double double_value = 0;
              if (data_value.value.get(double_value))
                max_value = std::max(max_value, double_value);
              return DataValue{max_value, data_value.qualifier, interval.first,
                               interval.second};
            };
      });

  aggregators.emplace(id::AggregateFunction_Start,
                      [](const DateTimeRange& interval, bool forward) {
                        return forward ? MakeFrontAggregator(interval)
                                       : MakeBackAggregator(interval);
                      });

  aggregators.emplace(id::AggregateFunction_End,
                      [](const DateTimeRange& interval, bool forward) {
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
                         const DateTimeRange& interval,
                         bool forward) {
  auto factory = GetAggregatorFactory(aggregate_type);
  return factory ? factory(interval, forward) : nullptr;
}

DateTime GetLocalAggregateStartTime() {
  DateTime result;
  DateTime::Exploded exploded = {2000, 1, 0, 1};
  if (!DateTime::FromLocalExploded(exploded, &result))
    result = DateTime::UnixEpoch();
  return result;
}

DateTimeRange GetAggregateInterval(DateTime time,
                                   DateTime origin_time,
                                   Duration interval) {
  assert(!interval.is_zero());
  assert(time >= origin_time);
  auto delta = time - origin_time;
  delta -= delta % interval;
  return {origin_time + delta, origin_time + delta + interval};
}

void AggregateState::Process(base::span<const scada::DataValue> raw_span) {
  while (!raw_span.empty()) {
    auto interval = scada::GetAggregateInterval(raw_span[0].source_timestamp,
                                                aggregation.start_time,
                                                aggregation.interval);

    auto count = forward ? UpperBound(raw_span, interval.second)
                         : ReverseUpperBound(raw_span, interval.first);
    assert(count != 0);

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
