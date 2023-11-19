#pragma once

#include "base/boost_log.h"
#include "base/constraints.h"
#include "base/format_time.h"
#include "base/interval_util.h"
#include "base/observer_list.h"
#include "common/data_value_traits.h"
#include "common/timed_data_util.h"
#include "timed_data/timed_data_util.h"
#include "timed_data/timed_data_view_dump.h"
#include "timed_data/timed_data_view_fwd.h"
#include "timed_data/timed_data_view_observer.h"

#include <optional>

template <typename T>
class BasicTimedDataView final {
 public:
  BasicTimedDataView() = default;
  ~BasicTimedDataView() { assert(!observers_.might_have_observers()); }

  BasicTimedDataView(const BasicTimedDataView&) = delete;
  BasicTimedDataView& operator=(const BasicTimedDataView&) = delete;

  using ObservedRangesUpdatedHandler = std::function<void(bool has_observers)>;

  void set_observed_ranges_updated_handler(
      ObservedRangesUpdatedHandler handler) {
    observed_ranges_updated_handler_ = std::move(handler);
  }

  const std::vector<T>& values() const { return values_; }

  // Returns a pointer instead of an optional for performance reasons.
  const T* GetValueAt(const scada::DateTime& time) const;

  // Returns false if no change was applied. That means there is another value
  // for the same timestamp with earlier server timestamp.
  // WARNING: Does NOT notify observers of updates.
  // TODO: Notify observers of updates.
  bool InsertOrUpdate(const T& value);

  // Moves `values` into the view. `values` must be sorted by timestamp.
  // Drops all values in the overlapped time range that were in the view before.
  void ReplaceRange(std::span<T> values);

  // WARNING: Does NOT notify observers of updates.
  // TODO: Notify observers of updates.
  void ClearRange(const scada::DateTimeRange& range);

  // Observation.

  void AddObserver(BasicTimedDataViewObserver<T>& observer,
                   const scada::DateTimeRange& range);
  void RemoveObserver(BasicTimedDataViewObserver<T>& observer);

  // TODO: Remove from public API.
  void NotifyUpdates(std::span<const T> values);

  // Readiness.

  const std::vector<scada::DateTimeRange>& ready_ranges() const {
    return ready_ranges_;
  }

  void AddReadyRange(const scada::DateTimeRange& range);

  // Finds a next observed range that's not covered by a ready range.
  std::optional<scada::DateTimeRange> FindNextGap() const;

  // Rest.

  void Dump(std::ostream& stream) const;

 private:
  void UpdateObservedRanges();

  inline static constexpr scada::DateTime timestamp(const T& value) {
    return TimedDataTraits<T>::timestamp(value);
  }

  base::ObserverList<BasicTimedDataViewObserver<T>> observers_;
  std::map<BasicTimedDataViewObserver<T>*, scada::DateTimeRange>
      observer_ranges_;

  std::vector<scada::DateTimeRange> observed_ranges_;
  std::vector<scada::DateTimeRange> ready_ranges_;

  ObservedRangesUpdatedHandler observed_ranges_updated_handler_;

  std::vector<T> values_;

  inline static BoostLogger logger_{LOG_NAME("TimedData")};
};

template <typename T>
inline const T* BasicTimedDataView<T>::GetValueAt(
    const scada::DateTime& time) const {
  auto i = LowerBound(values_, time);

  if (i != values_.size() && timestamp(values_[i]) == time) {
    return &values_[i];
  }

  if (i == 0) {
    return nullptr;
  }

  return &values_[i - 1];
}

template <typename T>
inline void BasicTimedDataView<T>::AddObserver(
    BasicTimedDataViewObserver<T>& observer,
    const scada::DateTimeRange& range) {
  assert(!range.second.is_null());
  assert(IsValidInterval(range));
  assert(range.first == kTimedDataCurrentOnly || !IsEmptyInterval(range));

  if (!observers_.HasObserver(&observer))
    observers_.AddObserver(&observer);

  bool inserted = observer_ranges_.insert_or_assign(&observer, range).second;

  LOG_INFO(logger_) << "Add observer" << LOG_TAG("New", inserted)
                    << LOG_TAG("From", FormatTime(range.first))
                    << LOG_TAG("To", FormatTime(range.second));

  UpdateObservedRanges();
}

template <typename T>
inline void BasicTimedDataView<T>::RemoveObserver(
    BasicTimedDataViewObserver<T>& observer) {
  observers_.RemoveObserver(&observer);

  scada::DateTimeRange range{kTimedDataCurrentOnly, kTimedDataCurrentOnly};
  if (auto i = observer_ranges_.find(&observer); i != observer_ranges_.end()) {
    range = i->second;
    observer_ranges_.erase(i);
  }

  LOG_INFO(logger_) << "Remove observer"
                    << LOG_TAG("From", FormatTime(range.first))
                    << LOG_TAG("To", FormatTime(range.second));

  UpdateObservedRanges();
}

template <typename T>
inline void BasicTimedDataView<T>::UpdateObservedRanges() {
  observed_ranges_.clear();

  for (const auto& [observer, range] : observer_ranges_) {
    if (range.first != kTimedDataCurrentOnly) {
      assert(!IsEmptyInterval(range));
      UnionIntervals(observed_ranges_, range);
    }
  }

  if (observed_ranges_updated_handler_) {
    observed_ranges_updated_handler_(!observed_ranges_.empty());
  }
}

template <typename T>
inline std::optional<scada::DateTimeRange> BasicTimedDataView<T>::FindNextGap()
    const {
  return FindFirstGap(observed_ranges_, ready_ranges_);
}

template <typename T>
inline void BasicTimedDataView<T>::AddReadyRange(
    const scada::DateTimeRange& range) {
  UnionIntervals(ready_ranges_, range);

  for (auto& o : observers_) {
    o.OnTimedDataReady();
  }
}

template <typename T>
inline void BasicTimedDataView<T>::NotifyUpdates(std::span<const T> values) {
  assert(!values.empty());
  assert(IsTimeSorted(values));

  for (auto& o : observers_)
    o.OnTimedDataUpdates(values);
}

template <typename T>
inline bool BasicTimedDataView<T>::InsertOrUpdate(const T& value) {
  ScopedInvariant values_sorted{[&] { return IsTimeSorted(values_); }};

  if (timestamp(value).is_null())
    return false;

  // An optimization for tail inserts.
  if (values_.empty() || timestamp(values_.back()) < timestamp(value)) {
    values_.emplace_back(value);
    return true;
  }

  auto i = LowerBound(values_, timestamp(value));
  if (i != values_.size() && timestamp(values_[i]) == timestamp(value)) {
    if (TimedDataTraits<T>::IsLesser(value, values_[i])) {
      return false;
    }
    values_[i] = value;
    return true;

  } else {
    values_.insert(values_.begin() + i, value);
    return true;
  }
}

template <typename T>
inline void BasicTimedDataView<T>::ClearRange(
    const scada::DateTimeRange& range) {
  assert(!range.first.is_null());
  assert(range.second.is_null() || range.first <= range.second);

  auto i = LowerBound(values_, range.first);
  auto j = range.second.is_null() ? values_.size()
                                  : UpperBound(values_, range.second);
  values_.erase(values_.begin() + i, values_.begin() + j);
}

template <typename T>
inline void BasicTimedDataView<T>::Dump(std::ostream& stream) const {
  stream << "BasicTimedDataView" << std::endl;
  stream << "Observers:" << std::endl;
  internal::Dump(stream, observer_ranges_);
  stream << "Observed ranges:" << std::endl;
  internal::Dump(stream, observed_ranges_);
  stream << "Ready ranges:" << std::endl;
  internal::Dump(stream, ready_ranges_);
  stream << "Value count: " << values_.size() << std::endl;
  if (values_.size() <= internal::kDumpMaxValueCount) {
    internal::DumpRange(stream, values_.begin(), values_.end());
  } else {
    stream << "First " << internal::kDumpMaxValueCount / 2 << ":" << std::endl;
    internal::DumpRange(stream, values_.begin(),
                        values_.begin() + internal::kDumpMaxValueCount / 2);
    stream << "Last " << internal::kDumpMaxValueCount / 2 << ":" << std::endl;
    internal::DumpRange(stream,
                        values_.end() - internal::kDumpMaxValueCount / 2,
                        values_.end());
  }
}

template <typename T>
inline void BasicTimedDataView<T>::ReplaceRange(std::span<T> values) {
  if (values.empty()) {
    return;
  }

  // Optimization: if all new values relate to the same position in history,
  // not overlapping other values, then insert them as whole.
  if (auto i = FindInsertPosition(values_, timestamp(values.front()),
                                  timestamp(values.back()))) {
    values_.insert(values_.begin() + *i,
                   std::make_move_iterator(values.begin()),
                   std::make_move_iterator(values.end()));
  } else {
    ReplaceSubrange(values_, values, [](const T& a, const T& b) {
      return timestamp(a) < timestamp(b);
    });
  }

  assert(IsTimeSorted(values_));
}
