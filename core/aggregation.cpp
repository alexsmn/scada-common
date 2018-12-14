#include "core/aggregation.h"

#include "core/data_value.h"
#include "core/standard_node_ids.h"

#include <algorithm>
#include <map>
#include <numeric>

namespace scada {

namespace {

struct AggregatorContext {
  scada::DateTime timestamp;
  size_t count;
  const scada::DataValue* values;
};

using Aggregator =
    std::function<scada::DataValue(const AggregatorContext& context)>;

scada::DateTime CeilAggregationTime(scada::DateTime time,
                                    scada::Duration interval) {
  auto origin = scada::DateTime::UnixEpoch();
  auto origin_delta = time - origin;
  origin_delta -= origin_delta % interval;
  return origin + origin_delta;
}

struct CompareVariants {
  bool operator()(const scada::Variant& left,
                  const scada::Variant& right) const {
    double left_double = 0;
    double right_double = 0;
    if (left.get(left_double) && right.get(right_double))
      return left_double < right_double;
    else
      return &left < &right;
  }
};

struct CompareDataValues {
  bool operator()(const scada::DataValue& left, const scada::DataValue& right) {
    return CompareVariants{}(left.value, right.value);
  }
};

scada::Double CalculateTotal(size_t count, const scada::DataValue* values) {
  return std::accumulate(
      values, values + count, 0.0,
      [](scada::Double prev_sum, const scada::DataValue& data_value) {
        if (auto* double_value = data_value.value.get_if<scada::Double>())
          return prev_sum + *double_value;
        else
          return prev_sum;
      });
}

std::map<scada::NodeId, Aggregator> BuildAggregatorMap() {
  std::map<scada::NodeId, Aggregator> aggregators;

  aggregators.emplace(
      scada::id::AggregateFunction_Total, [](const AggregatorContext& context) {
        auto total = CalculateTotal(context.count, context.values);
        return scada::DataValue{
            total, {}, context.timestamp, context.timestamp};
      });

  aggregators.emplace(
      scada::id::AggregateFunction_Average,
      [](const AggregatorContext& context) {
        auto total = CalculateTotal(context.count, context.values);
        return scada::DataValue{
            total / context.count, {}, context.timestamp, context.timestamp};
      });

  aggregators.emplace(
      scada::id::AggregateFunction_Count, [](const AggregatorContext& context) {
        return scada::DataValue{
            context.count, {}, context.timestamp, context.timestamp};
      });

  aggregators.emplace(
      scada::id::AggregateFunction_Minimum,
      [](const AggregatorContext& context) {
        auto i =
            std::min_element(context.values, context.values + context.count,
                             CompareDataValues{});
        return scada::DataValue{i->value, i->qualifier, context.timestamp,
                                context.timestamp};
      });

  aggregators.emplace(
      scada::id::AggregateFunction_Maximum,
      [](const AggregatorContext& context) {
        auto i =
            std::max_element(context.values, context.values + context.count,
                             CompareDataValues{});
        return scada::DataValue{i->value, i->qualifier, context.timestamp,
                                context.timestamp};
      });

  aggregators.emplace(
      scada::id::AggregateFunction_Start,
      [](const AggregatorContext& context) { return context.values[0]; });

  aggregators.emplace(scada::id::AggregateFunction_End,
                      [](const AggregatorContext& context) {
                        return context.values[context.count - 1];
                      });

  return aggregators;
}

Aggregator GetAggregator(const scada::NodeId& aggregator_id) {
  const auto kAggregators = BuildAggregatorMap();
  auto i = kAggregators.find(aggregator_id);
  return i != kAggregators.end() ? i->second : nullptr;
}

}  // namespace

std::vector<scada::DataValue> Aggregate(
    const std::vector<scada::DataValue>& values,
    const scada::AggregateFilter& aggregation) {
  assert(!aggregation.interval.is_zero());
  assert(!aggregation.aggregate_type.is_null());
  assert(
      std::is_sorted(values.begin(), values.end(),
                     [](const scada::DataValue& a, const scada::DataValue& b) {
                       return a.source_timestamp < b.source_timestamp;
                     }));
  assert(std::none_of(
      values.begin(), values.end(),
      [](const scada::DataValue& v) { return v.server_timestamp.is_null(); }));

  const auto aggregator = GetAggregator(aggregation.aggregate_type);
  if (!aggregator)
    return {};

  std::vector<scada::DataValue> result;

  auto start = values.begin();
  while (start != values.end()) {
    auto start_timestamp =
        CeilAggregationTime(start->source_timestamp, aggregation.interval);
    auto end_timestamp = start_timestamp + aggregation.interval;

    auto end = std::upper_bound(
        start, values.end(), end_timestamp,
        [](scada::DateTime timestamp, const scada::DataValue& data_value) {
          assert(!data_value.source_timestamp.is_null());
          return timestamp < data_value.source_timestamp;
        });
    assert(start < end);

    auto data_value = aggregator(AggregatorContext{
        start_timestamp, static_cast<size_t>(std::distance(start, end)),
        &*start});
    data_value.server_timestamp = start_timestamp;
    data_value.source_timestamp = start_timestamp;
    result.emplace_back(std::move(data_value));

    start = end;
  }

  return result;
}

}  // namespace scada
