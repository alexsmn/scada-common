#include "common/interval_util.h"

#include <gmock/gmock.h>

TEST(DateTimeUtil, Empty) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges;
  UnionIntervals(ranges, {10, 20});
  const std::vector<Range> kExpectedRanges = {{10, 20}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, InsertFirst) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}};
  UnionIntervals(ranges, {50, 80});
  const std::vector<Range> kExpectedRanges = {{50, 80}, {100, 200}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, ExpandFirst) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}};
  UnionIntervals(ranges, {50, 100});
  const std::vector<Range> kExpectedRanges = {{50, 200}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, ExpandFirstWithOverlapping) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}};
  UnionIntervals(ranges, {50, 200});
  const std::vector<Range> kExpectedRanges = {{50, 200}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, ExpandFirstBothSides) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}, {300, 400}};
  UnionIntervals(ranges, {50, 250});
  const std::vector<Range> kExpectedRanges = {{50, 250}, {300, 400}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, InsertLast) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}};
  UnionIntervals(ranges, {250, 300});
  const std::vector<Range> kExpectedRanges = {{100, 200}, {250, 300}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, ExpandLast) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}};
  UnionIntervals(ranges, {200, 300});
  const std::vector<Range> kExpectedRanges = {{100, 300}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, ExpandLastWithOverlapping) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}};
  UnionIntervals(ranges, {150, 300});
  const std::vector<Range> kExpectedRanges = {{100, 300}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, ExpandLastBothSides) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}, {300, 400}};
  UnionIntervals(ranges, {250, 450});
  const std::vector<Range> kExpectedRanges = {{100, 200}, {250, 450}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, MergeTwo) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}, {300, 400}};
  UnionIntervals(ranges, {150, 350});
  const std::vector<Range> kExpectedRanges = {{100, 400}};
  EXPECT_EQ(ranges, kExpectedRanges);
}

TEST(DateTimeUtil, MergeTwoAndExpandBothSides) {
  using Range = std::pair<int, int>;
  std::vector<Range> ranges = {{100, 200}, {300, 400}};
  UnionIntervals(ranges, {50, 450});
  const std::vector<Range> kExpectedRanges = {{50, 450}};
  EXPECT_EQ(ranges, kExpectedRanges);
}
