#include "timed_data/timed_data_view.h"

#include "timed_data/timed_data_buffer.h"

#include <gmock/gmock.h>

#include <tuple>
#include <vector>

using namespace testing;

namespace {

// A minimal sample type so the view/buffer can be exercised without pulling in
// the full scada::DataValue machinery. `seq` is a tiebreak among samples that
// share a timestamp, mirroring DataValue's server_timestamp tiebreak.
struct TestStruct {
  scada::DateTime timestamp;
  int value = 0;
  int seq = 0;
};

}  // namespace

template <>
struct TimedDataTraits<TestStruct> {
  static constexpr scada::DateTime timestamp(const TestStruct& v) {
    return v.timestamp;
  }

  static bool IsLesser(const TestStruct& a, const TestStruct& b) {
    return std::tie(a.timestamp, a.seq) < std::tie(b.timestamp, b.seq);
  }
};

namespace {

// A view observer that records what it was told, for assertions.
class RecordingObserver : public BasicTimedDataViewObserver<TestStruct> {
 public:
  void OnTimedDataUpdates(std::span<const TestStruct> values) override {
    ++update_count;
    last_update.assign(values.begin(), values.end());
  }
  void OnTimedDataReady() override { ++ready_count; }

  int update_count = 0;
  int ready_count = 0;
  std::vector<TestStruct> last_update;
};

// Distinct, ordered timestamps.
scada::DateTime At(int seconds) {
  static const scada::DateTime kBase = scada::DateTime::Now();
  return kBase + base::TimeDelta::FromSeconds(seconds);
}

std::vector<TestStruct> Samples(std::initializer_list<int> seconds) {
  std::vector<TestStruct> result;
  for (int s : seconds)
    result.push_back(TestStruct{.timestamp = At(s), .value = s});
  return result;
}

BasicTimedDataView<TestStruct> ViewOf(const std::vector<TestStruct>& v) {
  return BasicTimedDataView<TestStruct>{std::span<const TestStruct>{v}};
}

// ---- BasicTimedDataView (non-owning slice) ----

TEST(TimedDataViewTest, EmptyView) {
  BasicTimedDataView<TestStruct> view;
  EXPECT_TRUE(view.empty());
  EXPECT_EQ(view.size(), 0u);
  EXPECT_EQ(view.value_at(At(1)), nullptr);
  EXPECT_EQ(view.sample_at_or_before(At(1)), nullptr);
}

TEST(TimedDataViewTest, SliceInclusiveBounds) {
  auto data = Samples({0, 2, 4, 6, 8});
  auto view = ViewOf(data);

  // [2, 6] includes endpoints.
  auto slice = view.slice({At(2), At(6)});
  ASSERT_EQ(slice.size(), 3u);
  EXPECT_EQ(slice.front().value, 2);
  EXPECT_EQ(slice.back().value, 6);
}

TEST(TimedDataViewTest, SliceBetweenSamples) {
  auto data = Samples({0, 2, 4, 6, 8});
  auto view = ViewOf(data);

  // (1, 5) lands between samples: 2 and 4.
  auto slice = view.slice({At(1), At(5)});
  ASSERT_EQ(slice.size(), 2u);
  EXPECT_EQ(slice.front().value, 2);
  EXPECT_EQ(slice.back().value, 4);
}

TEST(TimedDataViewTest, FromAndUntilOpenEnds) {
  auto data = Samples({0, 2, 4, 6, 8});
  auto view = ViewOf(data);

  EXPECT_EQ(view.from(At(4)).size(), 3u);   // 4, 6, 8
  EXPECT_EQ(view.until(At(4)).size(), 3u);  // 0, 2, 4

  // A null bound is unbounded on that side.
  EXPECT_EQ(view.from(scada::DateTime{}).size(), 5u);
  EXPECT_EQ(view.until(scada::DateTime{}).size(), 5u);
}

TEST(TimedDataViewTest, ValueAtExactVersusAtOrBefore) {
  auto data = Samples({0, 2, 4});
  auto view = ViewOf(data);

  // Exact match.
  ASSERT_NE(view.value_at(At(2)), nullptr);
  EXPECT_EQ(view.value_at(At(2))->value, 2);
  // No exact match.
  EXPECT_EQ(view.value_at(At(3)), nullptr);

  // At-or-before falls back to the previous sample.
  ASSERT_NE(view.sample_at_or_before(At(3)), nullptr);
  EXPECT_EQ(view.sample_at_or_before(At(3))->value, 2);
  // Before the first sample: nothing.
  EXPECT_EQ(view.sample_at_or_before(At(-1)), nullptr);
  // After the last sample: the last one.
  ASSERT_NE(view.sample_at_or_before(At(99)), nullptr);
  EXPECT_EQ(view.sample_at_or_before(At(99))->value, 4);
}

TEST(TimedDataViewTest, Iteration) {
  auto data = Samples({1, 2, 3});
  auto view = ViewOf(data);
  int sum = 0;
  for (const auto& s : view)
    sum += s.value;
  EXPECT_EQ(sum, 6);
}

// ---- BasicTimedDataBuffer notification ----

TEST(TimedDataBufferTest, InsertNotifiesObserver) {
  BasicTimedDataBuffer<TestStruct> buffer;
  buffer.set_retention({.observed_window = false});
  RecordingObserver observer;
  buffer.AddObserver(observer, {At(0), At(10)});

  EXPECT_TRUE(buffer.InsertOrUpdate(TestStruct{.timestamp = At(5), .value = 5}));

  EXPECT_EQ(observer.update_count, 1);
  ASSERT_EQ(observer.last_update.size(), 1u);
  EXPECT_EQ(observer.last_update.front().value, 5);

  buffer.RemoveObserver(observer);
}

TEST(TimedDataBufferTest, BatchCoalescesNotifications) {
  BasicTimedDataBuffer<TestStruct> buffer;
  buffer.set_retention({.observed_window = false});
  RecordingObserver observer;
  buffer.AddObserver(observer, {At(0), At(10)});

  {
    auto batch = buffer.BeginUpdate();
    buffer.InsertOrUpdate(TestStruct{.timestamp = At(2), .value = 2});
    buffer.InsertOrUpdate(TestStruct{.timestamp = At(4), .value = 4});
    buffer.InsertOrUpdate(TestStruct{.timestamp = At(6), .value = 6});
    EXPECT_EQ(observer.update_count, 0);  // suppressed until the scope ends
  }

  // One coalesced notification spanning [2, 6].
  EXPECT_EQ(observer.update_count, 1);
  EXPECT_EQ(observer.last_update.size(), 3u);

  buffer.RemoveObserver(observer);
}

// ---- Retention (memory bounding) ----

TEST(TimedDataBufferTest, RetentionTrimsToObservedHull) {
  BasicTimedDataBuffer<TestStruct> buffer;
  RecordingObserver observer;
  // Observer only cares about [1, 3].
  buffer.AddObserver(observer, {At(1), At(3)});

  auto batch = Samples({0, 1, 2, 3, 4});
  buffer.ReplaceRange(std::span<TestStruct>{batch});

  // Samples outside the observed hull are evicted.
  ASSERT_EQ(buffer.values().size(), 3u);
  EXPECT_EQ(buffer.values().front().value, 1);
  EXPECT_EQ(buffer.values().back().value, 3);

  buffer.RemoveObserver(observer);
}

TEST(TimedDataBufferTest, CurrentOnlyObserverDropsHistory) {
  BasicTimedDataBuffer<TestStruct> buffer;
  RecordingObserver observer;
  // A current-only observer contributes no observed range.
  buffer.AddObserver(observer, {kTimedDataCurrentOnly, kTimedDataCurrentOnly});

  auto batch = Samples({0, 1, 2, 3, 4});
  buffer.ReplaceRange(std::span<TestStruct>{batch});

  // No observer wants history, so none is retained.
  EXPECT_TRUE(buffer.values().empty());

  buffer.RemoveObserver(observer);
}

TEST(TimedDataBufferTest, MaxSamplesBackstopDropsOldest) {
  BasicTimedDataBuffer<TestStruct> buffer;
  buffer.set_retention({.observed_window = false, .max_samples = 3});

  auto batch = Samples({0, 1, 2, 3, 4});
  buffer.ReplaceRange(std::span<TestStruct>{batch});

  // Only the newest three survive.
  ASSERT_EQ(buffer.values().size(), 3u);
  EXPECT_EQ(buffer.values().front().value, 2);
  EXPECT_EQ(buffer.values().back().value, 4);
}

TEST(TimedDataBufferTest, ReplaceRangeSanitizesUnsortedDuplicateAndNull) {
  BasicTimedDataBuffer<TestStruct> buffer;
  buffer.set_retention({.observed_window = false});

  // Unsorted, with a duplicate timestamp and a null-timestamp sample.
  std::vector<TestStruct> messy = {
      TestStruct{.timestamp = At(4), .value = 4},
      TestStruct{.timestamp = scada::DateTime{}, .value = -1},  // null: dropped
      TestStruct{.timestamp = At(1), .value = 1},
      TestStruct{.timestamp = At(4), .value = 40},  // duplicate of At(4)
      TestStruct{.timestamp = At(2), .value = 2},
  };
  buffer.ReplaceRange(std::span<TestStruct>{messy});

  ASSERT_EQ(buffer.values().size(), 3u);  // null dropped, At(4) de-duplicated
  EXPECT_EQ(buffer.values()[0].value, 1);
  EXPECT_EQ(buffer.values()[1].value, 2);
  EXPECT_EQ(TimedDataTraits<TestStruct>::timestamp(buffer.values()[2]), At(4));
}

TEST(TimedDataBufferTest, ReplaceRangeNotifiesObservers) {
  BasicTimedDataBuffer<TestStruct> buffer;
  buffer.set_retention({.observed_window = false});
  RecordingObserver observer;
  buffer.AddObserver(observer, {At(0), At(10)});

  auto batch = Samples({1, 2, 3});
  buffer.ReplaceRange(std::span<TestStruct>{batch});

  EXPECT_EQ(observer.update_count, 1);
  EXPECT_EQ(observer.last_update.size(), 3u);

  buffer.RemoveObserver(observer);
}

TEST(TimedDataBufferTest, ClearRangeRemovesSamples) {
  BasicTimedDataBuffer<TestStruct> buffer;
  buffer.set_retention({.observed_window = false});
  RecordingObserver observer;
  buffer.AddObserver(observer, {At(0), At(10)});

  auto batch = Samples({1, 2, 3, 4, 5});
  buffer.ReplaceRange(std::span<TestStruct>{batch});
  observer.update_count = 0;

  buffer.ClearRange({At(2), At(4)});

  // 2, 3, 4 removed; 1 and 5 remain.
  ASSERT_EQ(buffer.values().size(), 2u);
  EXPECT_EQ(buffer.values().front().value, 1);
  EXPECT_EQ(buffer.values().back().value, 5);

  // A standalone clear leaves the range empty, so there is nothing to hand an
  // observer (consumers re-read values() and ignore empty spans). Repopulation
  // and notification are the caller's job -- see the batch case below.
  EXPECT_EQ(observer.update_count, 0);

  buffer.RemoveObserver(observer);
}

TEST(TimedDataBufferTest, ClearThenRepopulateInBatchNotifiesOnce) {
  BasicTimedDataBuffer<TestStruct> buffer;
  buffer.set_retention({.observed_window = false});
  RecordingObserver observer;
  buffer.AddObserver(observer, {At(0), At(10)});

  auto batch = Samples({1, 2, 3, 4, 5});
  buffer.ReplaceRange(std::span<TestStruct>{batch});
  observer.update_count = 0;

  // The expression path's shape: clear a window then recompute it under one
  // scope, yielding a single coalesced notification of the new values.
  {
    auto scope = buffer.BeginUpdate();
    buffer.ClearRange({At(2), At(4)});
    buffer.InsertOrUpdate(TestStruct{.timestamp = At(2), .value = 20});
    buffer.InsertOrUpdate(TestStruct{.timestamp = At(3), .value = 30});
  }

  EXPECT_EQ(observer.update_count, 1);
  ASSERT_EQ(buffer.values().size(), 4u);  // 1, 20@2, 30@3, 5
  EXPECT_EQ(buffer.values()[1].value, 20);
  EXPECT_EQ(buffer.values()[2].value, 30);

  buffer.RemoveObserver(observer);
}

TEST(TimedDataBufferTest, InsertOrUpdateResolvesSameTimestampByTiebreak) {
  BasicTimedDataBuffer<TestStruct> buffer;
  buffer.set_retention({.observed_window = false});

  EXPECT_TRUE(buffer.InsertOrUpdate(
      TestStruct{.timestamp = At(1), .value = 5, .seq = 5}));

  // A sample at the same timestamp that sorts earlier is rejected.
  EXPECT_FALSE(buffer.InsertOrUpdate(
      TestStruct{.timestamp = At(1), .value = 1, .seq = 1}));
  ASSERT_EQ(buffer.values().size(), 1u);
  EXPECT_EQ(buffer.values().front().value, 5);

  // A sample that sorts later overwrites.
  EXPECT_TRUE(buffer.InsertOrUpdate(
      TestStruct{.timestamp = At(1), .value = 9, .seq = 9}));
  ASSERT_EQ(buffer.values().size(), 1u);
  EXPECT_EQ(buffer.values().front().value, 9);
}

TEST(TimedDataBufferTest, NestedBatchCoalescesToOneNotification) {
  BasicTimedDataBuffer<TestStruct> buffer;
  buffer.set_retention({.observed_window = false});
  RecordingObserver observer;
  buffer.AddObserver(observer, {At(0), At(10)});

  {
    auto outer = buffer.BeginUpdate();
    buffer.InsertOrUpdate(TestStruct{.timestamp = At(1), .value = 1});
    {
      auto inner = buffer.BeginUpdate();
      buffer.InsertOrUpdate(TestStruct{.timestamp = At(3), .value = 3});
    }
    // Inner scope closing must NOT flush while the outer scope is still open.
    EXPECT_EQ(observer.update_count, 0);
    buffer.InsertOrUpdate(TestStruct{.timestamp = At(5), .value = 5});
  }

  EXPECT_EQ(observer.update_count, 1);
  EXPECT_EQ(observer.last_update.size(), 3u);  // coalesced [1, 5]

  buffer.RemoveObserver(observer);
}

TEST(TimedDataBufferTest, NarrowingObserverRangeShrinksWorkingSet) {
  BasicTimedDataBuffer<TestStruct> buffer;
  RecordingObserver observer;
  buffer.AddObserver(observer, {At(0), At(4)});

  auto batch = Samples({0, 1, 2, 3, 4});
  buffer.ReplaceRange(std::span<TestStruct>{batch});
  ASSERT_EQ(buffer.values().size(), 5u);

  // Narrow the window; the buffer trims to the new hull on range change.
  buffer.AddObserver(observer, {At(1), At(2)});
  ASSERT_EQ(buffer.values().size(), 2u);
  EXPECT_EQ(buffer.values().front().value, 1);
  EXPECT_EQ(buffer.values().back().value, 2);

  buffer.RemoveObserver(observer);
}

}  // namespace
