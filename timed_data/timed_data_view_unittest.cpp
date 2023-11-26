#include "timed_data/timed_data_view.h"

#include "timed_data/timed_data_projection.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

struct TestStruct {
  scada::DateTime timestamp;
};

}  // namespace

template <>
struct TimedDataTraits<TestStruct> {
  static constexpr scada::DateTime timestamp(const TestStruct& v) {
    return v.timestamp;
  }

  static bool IsLesser(const TestStruct& a, const TestStruct& b) {
    return a.timestamp < b.timestamp;
  }
};

TEST(TimedDataViewTest, Test) {
  BasicTimedDataView<TestStruct> view;

  auto timestamp = scada::DateTime::Now();

  EXPECT_TRUE(view.InsertOrUpdate(TestStruct{.timestamp = timestamp}));

  BasicTimedDataProjection<TestStruct> projection{view};
  projection.SetTimeRange({timestamp, timestamp});

  // ASSERT_EQ(projection.size(), 1);
  // EXPECT_EQ(projection.at(0).timestamp, timestamp);
}
