#include "core/aggregation.h"

#include "core/data_value.h"
#include "core/standard_node_ids.h"

#include <algorithm>
#include <map>
#include <numeric>

namespace scada {

namespace {

using AggregatorFactory = std::function<Aggregator(DateTime start_time)>;

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

Double CalculateTotal(span<const DataValue> values) {
  return std::accumulate(values.begin(), values.end(), 0.0,
                         [](Double prev_sum, const DataValue& data_value) {
                           Double double_value = 0;
                           if (data_value.value.get(double_value))
                             return prev_sum + double_value;
                           else
                             return prev_sum;
                         });
}

std::map<NodeId, AggregatorFactory> BuildAggregatorFactoryMap() {
  std::map<NodeId, AggregatorFactory> aggregators;

  aggregators.emplace(id::AggregateFunction_Total, [](DateTime start_time) {
    Double total = 0;
    return [start_time, total](span<const DataValue> values) mutable {
      total += CalculateTotal(values);
      return DataValue{total, {}, start_time, start_time};
    };
  });

  aggregators.emplace(id::AggregateFunction_Average, [](DateTime start_time) {
    Double total = 0;
    size_t count = 0;
    return [start_time, total, count](span<const DataValue> values) mutable {
      total += CalculateTotal(values);
      count += values.size();
      assert(count != 0);
      return DataValue{total / count, {}, start_time, DateTime::Now()};
    };
  });

  aggregators.emplace(id::AggregateFunction_Count, [](DateTime start_time) {
    size_t count = 0;
    return [start_time, count](span<const DataValue> values) mutable {
      count += values.size();
      return DataValue{count, {}, start_time, DateTime::Now()};
    };
  });

  aggregators.emplace(id::AggregateFunction_Minimum, [](DateTime start_time) {
    auto min_value = std::numeric_limits<Double>::max();
    return [start_time, min_value](span<const DataValue> values) mutable {
      auto& data_value =
          *std::min_element(values.begin(), values.end(), CompareDataValues{});
      Double double_value = 0;
      if (data_value.value.get(double_value))
        min_value = std::min(min_value, double_value);
      return DataValue{min_value, data_value.qualifier, start_time,
                       DateTime::Now()};
    };
  });

  aggregators.emplace(id::AggregateFunction_Maximum, [](DateTime start_time) {
    auto max_value = std::numeric_limits<Double>::min();
    return [start_time, max_value](span<const DataValue> values) mutable {
      auto& data_value =
          *std::max_element(values.begin(), values.end(), CompareDataValues{});
      Double double_value = 0;
      if (data_value.value.get(double_value))
        max_value = std::max(max_value, double_value);
      return DataValue{max_value, data_value.qualifier, start_time,
                       DateTime::Now()};
    };
  });

  aggregators.emplace(id::AggregateFunction_Start, [](DateTime start_time) {
    DataValue start_data_value;
    return
        [start_time, start_data_value](span<const DataValue> values) mutable {
          if (start_data_value.is_null())
            start_data_value = values.front();
          return DataValue{start_data_value.value, start_data_value.qualifier,
                           start_time, DateTime::Now()};
        };
  });

  aggregators.emplace(id::AggregateFunction_End, [](DateTime start_time) {
    return [start_time](span<const DataValue> values) {
      const auto& end_data_value = values.back();
      return DataValue{end_data_value.value, end_data_value.qualifier,
                       start_time, DateTime::Now()};
    };
  });

  return aggregators;
}

AggregatorFactory GetAggregatorFactory(const NodeId& aggregate_type) {
  const auto kAggregators = BuildAggregatorFactoryMap();
  auto i = kAggregators.find(aggregate_type);
  return i != kAggregators.end() ? i->second : nullptr;
}

}  // namespace

Aggregator GetAggregator(const NodeId& aggregate_type, DateTime start_time) {
  auto factory = GetAggregatorFactory(aggregate_type);
  return factory ? factory(start_time) : nullptr;
}

DateTime GetAggregateStartTime(DateTime time, Duration interval) {
  assert(!interval.is_zero());
  auto origin = DateTime::UnixEpoch();
  auto origin_delta = time - origin;
  origin_delta -= origin_delta % interval;
  return origin + origin_delta;
}

std::vector<DataValue> AggregateRange(span<const DataValue> values,
                                      const AggregateFilter& aggregation) {
  assert(!aggregation.interval.is_zero());
  assert(!aggregation.aggregate_type.is_null());
  assert(std::is_sorted(values.begin(), values.end(),
                        [](const DataValue& a, const DataValue& b) {
                          return a.source_timestamp < b.source_timestamp;
                        }));
  assert(std::none_of(values.begin(), values.end(), [](const DataValue& v) {
    return v.server_timestamp.is_null();
  }));

  std::vector<DataValue> result;

  auto start = values.begin();
  while (start != values.end()) {
    auto start_time =
        GetAggregateStartTime(start->source_timestamp, aggregation.interval);
    auto end_time = start_time + aggregation.interval;

    auto end =
        std::upper_bound(start, values.end(), end_time,
                         [](DateTime timestamp, const DataValue& data_value) {
                           assert(!data_value.source_timestamp.is_null());
                           return timestamp < data_value.source_timestamp;
                         });
    assert(start < end);

    const auto aggregator =
        GetAggregator(aggregation.aggregate_type, start_time);
    if (!aggregator)
      break;

    auto data_value = aggregator(span{&*start, &*end});
    result.emplace_back(std::move(data_value));

    start = end;
  }

  return result;
}

}  // namespace scada
