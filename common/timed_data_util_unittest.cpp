#include "common/timed_data_util.h"

#include "common/data_value_traits.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(TimedDataUtil, ReplaceSubrange_Start) {
  std::vector<int> values = {38, 40, 50, 60, 70, 80};
  std::vector<int> updates = {30, 35, 40, 45, 50};
  ReplaceSubrange(values, {updates.data(), updates.size()}, std::less{});
  std::vector<int> expected = {30, 35, 40, 45, 50, 60, 70, 80};
  EXPECT_EQ(expected, values);
}

TEST(TimedDataUtil, ReplaceSubrange_Middle) {
  std::vector<int> values = {10, 20, 30, 40, 50, 60, 70, 80};
  std::vector<int> updates = {30, 35, 40, 45, 50};
  ReplaceSubrange(values, {updates.data(), updates.size()}, std::less{});
  std::vector<int> expected = {10, 20, 30, 35, 40, 45, 50, 60, 70, 80};
  EXPECT_EQ(expected, values);
}

TEST(TimedDataUtil, ReplaceSubrange_End) {
  std::vector<int> values = {10, 20, 30, 40, 42};
  std::vector<int> updates = {30, 35, 40, 45, 50};
  ReplaceSubrange(values, {updates.data(), updates.size()}, std::less{});
  std::vector<int> expected = {10, 20, 30, 35, 40, 45, 50};
  EXPECT_EQ(expected, values);
}
