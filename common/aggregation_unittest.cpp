#include "common/aggregation.h"

#include "scada/aggregate_filter.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <optional>
#include <vector>

using namespace std::chrono_literals;

namespace scada {
namespace {

const Time kStart = Time{} + 1000h;
const TimeRange kHour{kStart, kStart + 1h};

DataValue Sample(Double value, Duration offset, Qualifier qualifier = {}) {
  return DataValue{value, qualifier, kStart + offset, kStart + offset};
}

Double ValueOf(const DataValue& data_value) {
  Double result = 0;
  EXPECT_TRUE(data_value.value.get(result));
  return result;
}

// Runs one aggregate over |values| in archive order, either all at once or one
// sample per call (which is how AggregateMonitoredItem feeds it).
DataValue Aggregate(NumericId aggregate_type,
                    std::vector<DataValue> values,
                    bool forward = true,
                    bool one_by_one = false) {
  if (!forward)
    std::ranges::reverse(values);
  Aggregator aggregator = GetAggregator(NodeId{aggregate_type}, kHour, forward);
  EXPECT_TRUE(aggregator);
  DataValue result;
  if (one_by_one) {
    for (const auto& value : values)
      result = aggregator(std::span{&value, 1});
  } else {
    result = aggregator(values);
  }
  EXPECT_FALSE(result.is_null());
  EXPECT_EQ(result.source_timestamp, kHour.first);
  EXPECT_EQ(result.server_timestamp, kHour.second);
  return result;
}

bool IsNoData(const DataValue& data_value) {
  return data_value.value.is_null() && data_value.qualifier.bad() &&
         !IsGood(data_value.status_code);
}

const Qualifier kBad{Qualifier::BAD};
const Qualifier kOffline{Qualifier::OFFLINE};

TEST(AggregationTest, MaximumOfNegativeValuesIsTheLargestOfThem) {
  // Seeded with numeric_limits::min() (the smallest positive Double), this
  // reported ~0 for an all-negative interval.
  EXPECT_EQ(-2.0, ValueOf(Aggregate(id::AggregateFunction_Maximum,
                                    {Sample(-5, 0min), Sample(-2, 10min),
                                     Sample(-7, 20min)})));
  EXPECT_EQ(-2.0, ValueOf(Aggregate(
                      id::AggregateFunction_Maximum,
                      {Sample(-5, 0min), Sample(-2, 10min), Sample(-7, 20min)},
                      /*forward=*/true, /*one_by_one=*/true)));
}

TEST(AggregationTest, MinimumAndMaximumSkipNonGoodValues) {
  std::vector<DataValue> values = {Sample(5, 0min), Sample(100, 10min, kBad),
                                   Sample(-100, 20min, kOffline),
                                   Sample(7, 30min)};
  EXPECT_EQ(5.0, ValueOf(Aggregate(id::AggregateFunction_Minimum, values)));
  EXPECT_EQ(7.0, ValueOf(Aggregate(id::AggregateFunction_Maximum, values)));
}

TEST(AggregationTest, AverageIsTheArithmeticMeanOfGoodValues) {
  EXPECT_EQ(3.0, ValueOf(Aggregate(id::AggregateFunction_Average,
                                   {Sample(2, 0min), Sample(1000, 1min, kBad),
                                    Sample(4, 59min)})));
}

TEST(AggregationTest, CountCountsGoodValuesOnly) {
  DataValue count =
      Aggregate(id::AggregateFunction_Count,
                {Sample(1, 0min), Sample(2, 1min, kBad), Sample(3, 2min)});
  UInt64 result = 0;
  ASSERT_TRUE(count.value.get(result));
  EXPECT_EQ(2u, result);
}

TEST(AggregationTest, TotalIsTheStepIntegralInValueSeconds) {
  // 10 for 15 minutes, then 30 for the remaining 45: 10*900 + 30*2700.
  std::vector<DataValue> values = {Sample(10, 0min), Sample(30, 15min)};
  constexpr Double kArea = 10 * 900 + 30 * 2700;
  EXPECT_DOUBLE_EQ(kArea,
                   ValueOf(Aggregate(id::AggregateFunction_Total, values)));
  EXPECT_DOUBLE_EQ(kArea,
                   ValueOf(Aggregate(id::AggregateFunction_Total2, values)));
  EXPECT_DOUBLE_EQ(kArea, ValueOf(Aggregate(id::AggregateFunction_Total, values,
                                            /*forward=*/false)));
  EXPECT_DOUBLE_EQ(kArea, ValueOf(Aggregate(id::AggregateFunction_Total, values,
                                            /*forward=*/true,
                                            /*one_by_one=*/true)));
}

TEST(AggregationTest, TimeAverageWeightsEachValueByHowLongItHeld) {
  // The arithmetic mean of {10, 30} is 20; the value 30 held three times as
  // long, so the time average is 25.
  std::vector<DataValue> values = {Sample(10, 0min), Sample(30, 15min)};
  EXPECT_DOUBLE_EQ(
      25.0, ValueOf(Aggregate(id::AggregateFunction_TimeAverage2, values)));
  EXPECT_DOUBLE_EQ(25.0, ValueOf(Aggregate(id::AggregateFunction_TimeAverage2,
                                           values, /*forward=*/false)));
}

TEST(AggregationTest, WithoutAStartBoundTimeBeforeTheFirstSampleIsNotCovered) {
  // Fed no value from before the interval, the aggregator does not know what
  // held before 30 min, so only the last half hour counts.
  std::vector<DataValue> values = {Sample(4, 30min)};
  EXPECT_DOUBLE_EQ(
      4.0, ValueOf(Aggregate(id::AggregateFunction_TimeAverage2, values)));
  EXPECT_DOUBLE_EQ(4.0 * 1800,
                   ValueOf(Aggregate(id::AggregateFunction_Total, values)));
}

TEST(AggregationTest, ABadSampleEndsTheStepAndItsDurationIsLeftOut) {
  // Good 10 for 20 min, Bad for 20 min, Good 40 for 20 min.
  std::vector<DataValue> values = {Sample(10, 0min), Sample(999, 20min, kBad),
                                   Sample(40, 40min)};
  for (bool forward : {true, false}) {
    EXPECT_DOUBLE_EQ(
        10 * 1200 + 40 * 1200,
        ValueOf(Aggregate(id::AggregateFunction_Total, values, forward)));
    EXPECT_DOUBLE_EQ(25.0, ValueOf(Aggregate(id::AggregateFunction_TimeAverage2,
                                             values, forward)));
  }
}

TEST(AggregationTest, AnIntervalWithoutGoodValuesYieldsNoData) {
  std::vector<DataValue> values = {Sample(1, 0min, kBad)};
  for (NumericId aggregate :
       {id::AggregateFunction_Average, id::AggregateFunction_Minimum,
        id::AggregateFunction_Maximum, id::AggregateFunction_Total,
        id::AggregateFunction_TimeAverage2}) {
    EXPECT_TRUE(IsNoData(Aggregate(aggregate, values))) << aggregate;
  }
}

TEST(AggregationTest, AStartBoundCoversTheTimeBeforeTheFirstSample) {
  // 7 was recorded before the interval and held until 4 replaced it at
  // 30 min (Part 13 §3.1.9 simple bounding values).
  const DataValue bound = Sample(7, -10min);
  for (bool forward : {true, false}) {
    for (NumericId aggregate :
         {id::AggregateFunction_TimeAverage2, id::AggregateFunction_Total}) {
      Aggregator aggregator = GetAggregator(NodeId{aggregate}, kHour, forward);
      ASSERT_TRUE(aggregator);
      DataValue inside = Sample(4, 30min);
      DataValue result;
      if (forward) {
        aggregator(std::span{&bound, 1});
        result = aggregator(std::span{&inside, 1});
      } else {
        aggregator(std::span{&inside, 1});
        result = aggregator(std::span{&bound, 1});
      }
      const Double expected = aggregate == id::AggregateFunction_Total
                                  ? 7.0 * 1800 + 4.0 * 1800
                                  : 5.5;
      EXPECT_DOUBLE_EQ(expected, ValueOf(result))
          << aggregate << (forward ? " forward" : " backward");
    }
  }
}

TEST(AggregationTest, ABadStartBoundCoversNothing) {
  // Part 13 Table 55: a Bad bound is "treated as any other Bad value".
  const DataValue bound = Sample(1000, -10min, kBad);
  Aggregator aggregator = GetAggregator(
      NodeId{id::AggregateFunction_TimeAverage2}, kHour, /*forward=*/true);
  aggregator(std::span{&bound, 1});
  DataValue inside = Sample(4, 30min);
  EXPECT_DOUBLE_EQ(4.0, ValueOf(aggregator(std::span{&inside, 1})));
}

TEST(AggregationTest, OnlyTheTimeWeightedAggregatesUseAStartBound) {
  EXPECT_TRUE(UsesStartBound(NodeId{id::AggregateFunction_TimeAverage2}));
  EXPECT_TRUE(UsesStartBound(NodeId{id::AggregateFunction_Total}));
  EXPECT_TRUE(UsesStartBound(NodeId{id::AggregateFunction_Total2}));
  for (NumericId aggregate :
       {id::AggregateFunction_Average, id::AggregateFunction_Minimum,
        id::AggregateFunction_Maximum, id::AggregateFunction_Count,
        id::AggregateFunction_Start, id::AggregateFunction_End}) {
    EXPECT_FALSE(UsesStartBound(NodeId{aggregate})) << aggregate;
  }
}

// Runs AggregateState over |raw| (ascending) in the given direction, with
// 30-minute intervals, and returns the results in ascending order.
struct StateRun {
  NumericId aggregate_type = id::AggregateFunction_TimeAverage2;
  bool forward = true;
  std::optional<DataValue> start_bound;
  Time fill_from = kNullTime;
  Time fill_until = kNullTime;
  size_t max_count = std::numeric_limits<size_t>::max();

  std::vector<DataValue> Run(std::vector<DataValue> raw) const {
    const AggregateFilter filter{.start_time = kStart,
                                 .interval = 30min,
                                 .aggregate_type = NodeId{aggregate_type}};
    std::vector<DataValue> results;
    AggregateState state{.forward = forward,
                         .aggregation = filter,
                         .data_values = results,
                         .start_bound = start_bound,
                         .fill_from = fill_from,
                         .fill_until = fill_until,
                         .max_count = max_count};
    if (!forward)
      std::ranges::reverse(raw);
    // One value per call, the way pages arrive from the archive.
    for (const auto& value : raw)
      state.Process(std::span{&value, 1});
    state.Finish();
    if (!forward)
      std::ranges::reverse(results);
    return results;
  }
};

std::vector<Double> ValuesOf(const std::vector<DataValue>& data_values) {
  std::vector<Double> result;
  for (const auto& data_value : data_values)
    result.push_back(ValueOf(data_value));
  return result;
}

std::vector<Time> StartsOf(const std::vector<DataValue>& data_values) {
  std::vector<Time> result;
  for (const auto& data_value : data_values)
    result.push_back(data_value.source_timestamp);
  return result;
}

TEST(AggregationTest, AggregateStateCarriesTheValueIntoTheNextInterval) {
  // 30 was recorded at 15 min and held until -6 replaced it at 40 min, so the
  // second interval is 30 for 10 min and -6 for 20. It used to start at its
  // first sample and report -6. (A sample exactly on a boundary is counted in
  // the earlier interval by AggregateState's UpperBound.)
  std::vector<DataValue> raw = {Sample(10, 0min), Sample(30, 15min),
                                Sample(-6, 40min)};
  for (bool forward : {true, false}) {
    auto results = StateRun{.forward = forward}.Run(raw);
    ASSERT_EQ(2u, results.size()) << forward;
    EXPECT_DOUBLE_EQ(20.0, ValueOf(results[0])) << forward;
    EXPECT_DOUBLE_EQ((30.0 * 10 - 6.0 * 20) / 30, ValueOf(results[1]))
        << forward;
  }
}

TEST(AggregationTest, AggregateStateProducesTheIntervalsAValueHeldThrough) {
  // Nothing was recorded between 10 min and 100 min: 5 held through the
  // 30-60 and 60-90 intervals, and into the 90-120 one.
  std::vector<DataValue> raw = {Sample(5, 10min), Sample(9, 100min)};
  for (bool forward : {true, false}) {
    auto results = StateRun{.forward = forward}.Run(raw);
    EXPECT_THAT(StartsOf(results),
                testing::ElementsAre(kStart, kStart + 30min, kStart + 60min,
                                     kStart + 90min))
        << forward;
    EXPECT_THAT(ValuesOf(results),
                testing::ElementsAre(5.0, 5.0, 5.0, (5.0 * 10 + 9.0 * 20) / 30))
        << forward;

    auto totals = StateRun{.aggregate_type = id::AggregateFunction_Total,
                           .forward = forward}
                      .Run(raw);
    EXPECT_THAT(ValuesOf(totals),
                testing::ElementsAre(5.0 * 1200, 5.0 * 1800, 5.0 * 1800,
                                     5.0 * 600 + 9.0 * 1200))
        << forward;
  }
}

TEST(AggregationTest, AggregateStateStartsFromTheValueBeforeTheRead) {
  // 2 was recorded before the read starts; it held through the first interval
  // and the first 10 minutes of the second.
  auto results =
      StateRun{.start_bound = Sample(2, -5min), .fill_from = kStart}.Run(
          {Sample(8, 40min)});
  EXPECT_THAT(StartsOf(results), testing::ElementsAre(kStart, kStart + 30min));
  EXPECT_THAT(ValuesOf(results),
              testing::ElementsAre(2.0, (2.0 * 10 + 8.0 * 20) / 30));
}

TEST(AggregationTest, AggregateStateExtendsTheLastValueToTheEndOfTheRead) {
  auto results = StateRun{.fill_until = kStart + 80min}.Run({Sample(3, 10min)});
  EXPECT_THAT(StartsOf(results),
              testing::ElementsAre(kStart, kStart + 30min, kStart + 60min));
  EXPECT_THAT(ValuesOf(results), testing::ElementsAre(3.0, 3.0, 3.0));

  // Nothing recorded during the read at all: the value from before it holds.
  auto quiet = StateRun{.start_bound = Sample(6, -1h),
                        .fill_from = kStart,
                        .fill_until = kStart + 60min}
                   .Run({});
  EXPECT_THAT(ValuesOf(quiet), testing::ElementsAre(6.0, 6.0));
}

TEST(AggregationTest, AggregateStateLeavesOtherAggregatesAlone) {
  // Maximum looks only at values recorded inside an interval, so the empty
  // intervals are not produced and nothing from before the read counts.
  auto results = StateRun{.aggregate_type = id::AggregateFunction_Maximum,
                          .start_bound = Sample(100, -5min),
                          .fill_from = kStart,
                          .fill_until = kStart + 3h}
                     .Run({Sample(5, 10min), Sample(9, 100min)});
  EXPECT_THAT(StartsOf(results), testing::ElementsAre(kStart, kStart + 90min));
  EXPECT_THAT(ValuesOf(results), testing::ElementsAre(5.0, 9.0));
}

TEST(AggregationTest, AggregateStateStopsFillingAtMaxCount) {
  // A long quiet stretch with short intervals must not grow the output
  // without limit. The caller returns the first |max_count| values and pages
  // on from the last of them, so what follows the cut is recomputed then.
  auto results =
      StateRun{.max_count = 3}.Run({Sample(1, 0min), Sample(2, 24h)});
  EXPECT_THAT(StartsOf(results),
              testing::ElementsAre(kStart, kStart + 30min, kStart + 60min,
                                   kStart + 24h));
}

}  // namespace
}  // namespace scada
