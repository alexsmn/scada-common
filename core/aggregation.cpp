#include "core/aggregation.h"

#include "core/data_value.h"
#include "core/standard_node_ids.h"

#include <algorithm>
#include <map>
#include <numeric>

namespace scada {

namespace {

struct AggregatorContext {
  DateTime start_time;
  DateTime server_timestamp;
  span<const DataValue> values;
};

using Aggregator = std::function<DataValue(const AggregatorContext& context)>;

DateTime CeilAggregationTime(DateTime time, Duration interval) {
  auto origin = DateTime::UnixEpoch();
  auto origin_delta = time - origin;
  origin_delta -= origin_delta % interval;
  return origin + origin_delta;
}

struct CompareVariants {
  bool operator()(const Variant& left, const Variant& right) const {
    double left_double = 0;
    double right_double = 0;
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
  return std::accumulate(
      values.begin(), values.end(), 0.0,
      [](Double prev_sum, const DataValue& data_value) {
        if (auto* double_value = data_value.value.get_if<Double>())
          return prev_sum + *double_value;
        else
          return prev_sum;
      });
}

std::map<NodeId, Aggregator> BuildAggregatorMap() {
  std::map<NodeId, Aggregator> aggregators;

  aggregators.emplace(
      id::AggregateFunction_Total, [](const AggregatorContext& context) {
        auto total = CalculateTotal(context.values);
        return DataValue{total, {}, context.start_time, context.start_time};
      });

  aggregators.emplace(id::AggregateFunction_Average,
                      [](const AggregatorContext& context) {
                        auto total = CalculateTotal(context.values);
                        return DataValue{total / context.values.size(),
                                         {},
                                         context.start_time,
                                         context.server_timestamp};
                      });

  aggregators.emplace(id::AggregateFunction_Count,
                      [](const AggregatorContext& context) {
                        return DataValue{context.values.size(),
                                         {},
                                         context.start_time,
                                         context.server_timestamp};
                      });

  aggregators.emplace(
      id::AggregateFunction_Minimum, [](const AggregatorContext& context) {
        auto& data_value = *std::min_element(
            context.values.begin(), context.values.end(), CompareDataValues{});
        return DataValue{data_value.value, data_value.qualifier,
                         context.start_time, context.server_timestamp};
      });

  aggregators.emplace(
      id::AggregateFunction_Maximum, [](const AggregatorContext& context) {
        auto& data_value = *std::max_element(
            context.values.begin(), context.values.end(), CompareDataValues{});
        return DataValue{data_value.value, data_value.qualifier,
                         context.start_time, context.server_timestamp};
      });

  aggregators.emplace(
      id::AggregateFunction_Start, [](const AggregatorContext& context) {
        auto& data_value = context.values.back();
        return DataValue{data_value.value, data_value.qualifier,
                         context.start_time, context.server_timestamp};
      });

  aggregators.emplace(
      id::AggregateFunction_End, [](const AggregatorContext& context) {
        auto& data_value = context.values.back();
        return DataValue{data_value.value, data_value.qualifier,
                         context.start_time, context.server_timestamp};
      });

  return aggregators;
}

Aggregator GetAggregator(const NodeId& aggregator_id) {
  const auto kAggregators = BuildAggregatorMap();
  auto i = kAggregators.find(aggregator_id);
  return i != kAggregators.end() ? i->second : nullptr;
}

}  // namespace

std::vector<DataValue> Aggregate(span<const DataValue> values,
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

  const auto aggregator = GetAggregator(aggregation.aggregate_type);
  if (!aggregator)
    return {};

  std::vector<DataValue> result;

  auto start = values.begin();
  while (start != values.end()) {
    auto start_time =
        CeilAggregationTime(start->source_timestamp, aggregation.interval);
    auto end_time = start_time + aggregation.interval;

    auto end =
        std::upper_bound(start, values.end(), end_time,
                         [](DateTime timestamp, const DataValue& data_value) {
                           assert(!data_value.source_timestamp.is_null());
                           return timestamp < data_value.source_timestamp;
                         });
    assert(start < end);

    auto data_value = aggregator(
        AggregatorContext{start_time, DateTime::Now(), span(&*start, &*end)});
    result.emplace_back(std::move(data_value));

    start = end;
  }

  return result;
}

}  // namespace scada
