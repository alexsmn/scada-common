#include "timed_data/timed_data_util.h"

#include <gmock/gmock.h>

namespace rt {

TEST(TimedDataUtil, FindLastGap1) {
  std::vector<Interval<int>> ranges = {
      {20, 50},
  };
  std::vector<Interval<int>> ready_ranges = {
      {30, 50},
  };
  auto gap = FindLastGap(ranges, ready_ranges);
  ASSERT_TRUE(gap);
  EXPECT_EQ(gap->first, 20);
  EXPECT_EQ(gap->second, 30);
}

TEST(TimedDataUtil, FindLastGap2) {
  std::vector<Interval<int>> ranges = {
      {20, 50},
  };
  std::vector<Interval<int>> ready_ranges = {
      {30, 40},
  };
  auto gap = FindLastGap(ranges, ready_ranges);
  ASSERT_TRUE(gap);
  EXPECT_EQ(gap->first, 40);
  EXPECT_EQ(gap->second, 50);
}

TEST(TimedDataUtil, FindLastGap3) {
  std::vector<Interval<int>> ranges = {
      {20, 40},
  };
  std::vector<Interval<int>> ready_ranges = {
      {30, 50},
  };
  auto gap = FindLastGap(ranges, ready_ranges);
  ASSERT_TRUE(gap);
  EXPECT_EQ(gap->first, 20);
  EXPECT_EQ(gap->second, 30);
}

TEST(TimedDataUtil, FindLastGap4) {
  std::vector<Interval<int>> ranges = {
      {20, 40},
  };
  std::vector<Interval<int>> ready_ranges = {
      {10, 50},
  };
  auto gap = FindLastGap(ranges, ready_ranges);
  ASSERT_FALSE(gap);
}

TEST(TimedDataUtil, FindLastGap5) {
  std::vector<Interval<int>> ranges = {
      {10, 20},
  };
  std::vector<Interval<int>> ready_ranges = {
      {10, 20},
  };
  auto gap = FindLastGap(ranges, ready_ranges);
  ASSERT_FALSE(gap);
}

TEST(TimedDataUtil, FindFirstGap1) {
  std::vector<Interval<int>> ranges = {
      {20, 50},
  };
  std::vector<Interval<int>> ready_ranges = {
      {20, 40},
  };
  auto gap = FindFirstGap(ranges, ready_ranges);
  ASSERT_TRUE(gap);
  EXPECT_EQ(gap->first, 40);
  EXPECT_EQ(gap->second, 50);
}

TEST(TimedDataUtil, FindFirstGap2) {
  std::vector<Interval<int>> ranges = {
      {20, 50},
  };
  std::vector<Interval<int>> ready_ranges = {
      {30, 40},
  };
  auto gap = FindFirstGap(ranges, ready_ranges);
  ASSERT_TRUE(gap);
  EXPECT_EQ(gap->first, 20);
  EXPECT_EQ(gap->second, 30);
}

TEST(TimedDataUtil, FindFirstGap3) {
  std::vector<Interval<int>> ranges = {
      {20, 40},
  };
  std::vector<Interval<int>> ready_ranges = {
      {10, 30},
  };
  auto gap = FindFirstGap(ranges, ready_ranges);
  ASSERT_TRUE(gap);
  EXPECT_EQ(gap->first, 30);
  EXPECT_EQ(gap->second, 40);
}

TEST(TimedDataUtil, FindFirstGap4) {
  std::vector<Interval<int>> ranges = {
      {20, 40},
  };
  std::vector<Interval<int>> ready_ranges = {
      {10, 50},
  };
  auto gap = FindFirstGap(ranges, ready_ranges);
  ASSERT_FALSE(gap);
}

TEST(TimedDataUtil, FindFirstGap5) {
  std::vector<Interval<int>> ranges = {
      {10, 20},
  };
  std::vector<Interval<int>> ready_ranges = {
      {10, 20},
  };
  auto gap = FindFirstGap(ranges, ready_ranges);
  ASSERT_FALSE(gap);
}

}  // namespace rt
