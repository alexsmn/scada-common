#include "common/aggregation.h"

#include "scada/aggregate_filter.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

#include <algorithm>
#include <chrono>
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

TEST(AggregationTest, TimeBeforeTheFirstSampleIsNotCovered) {
  // The value in force before 30 min is unknown to the aggregator, so only the
  // last half hour counts.
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

TEST(AggregationTest, AggregateStateSplitsRawValuesIntoIntervals) {
  const AggregateFilter filter{
      .start_time = kStart,
      .interval = 30min,
      .aggregate_type = NodeId{id::AggregateFunction_TimeAverage2}};
  std::vector<DataValue> results;
  AggregateState state{
      .forward = true, .aggregation = filter, .data_values = results};
  // A sample exactly on a boundary is counted in the earlier interval by
  // AggregateState (UpperBound), so the second interval starts at 40 min.
  std::vector<DataValue> raw = {Sample(10, 0min), Sample(30, 15min),
                                Sample(-6, 40min)};
  state.Process(raw);
  state.Finish();

  ASSERT_EQ(2u, results.size());
  EXPECT_DOUBLE_EQ(20.0, ValueOf(results[0]));
  EXPECT_DOUBLE_EQ(-6.0, ValueOf(results[1]));
}

}  // namespace
}  // namespace scada
