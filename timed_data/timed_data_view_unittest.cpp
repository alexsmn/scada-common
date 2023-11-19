#include "timed_data/timed_data_view.h"

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
  BasicTimedDataView<TestStruct> timed_data_view;

  EXPECT_TRUE(timed_data_view.InsertOrUpdate(
      TestStruct{.timestamp = scada::DateTime::Now()}));
}
