#pragma once

#include "base/boost_log.h"
#include "base/constraints.h"
#include "base/format_time.h"
#include "base/interval_util.h"
#include "base/observer_list.h"
#include "timed_data/timed_data_dump_util.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_util.h"
#include "timed_data/timed_data_view_observer.h"

class TimedDataView final {
 public:
  TimedDataView() = default;
  ~TimedDataView() { assert(!observers_.might_have_observers()); }

  TimedDataView(const TimedDataView&) = delete;
  TimedDataView& operator=(const TimedDataView&) = delete;

  const DataValues& values() const { return values_; }

  scada::DataValue GetValueAt(const base::Time& time) const;

  // Merge requested data with existing real-time data by collection time.
  void ReplaceRange(std::span<scada::DataValue> values);

  void ClearRange(const scada::DateTimeRange& range);

  // Returns false if there is another value for the same timestamp with earlier
  // server timestamp.
  bool InsertOrUpdate(const scada::DataValue& value);

  const std::vector<scada::DateTimeRange>& ready_ranges() const {
    return ready_ranges_;
  }

  void AddReadyRange(const scada::DateTimeRange& range);

  const base::ObserverList<TimedDataViewObserver>& observers() const {
    return observers_;
  }

  const std::vector<scada::DateTimeRange>& observed_ranges() const {
    return observed_ranges_;
  }

  void AddObserver(TimedDataViewObserver& observer,
                   const scada::DateTimeRange& range);
  void RemoveObserver(TimedDataViewObserver& observer);

  void UpdateObservedRanges();
  std::optional<scada::DateTimeRange> FindNextGap() const;

  void NotifyTimedDataCorrection(size_t count, const scada::DataValue* tvqs);
  void NotifyDataReady();

  void Dump(std::ostream& stream) const;

 private:
  base::ObserverList<TimedDataViewObserver> observers_;
  std::map<TimedDataViewObserver*, scada::DateTimeRange> observer_ranges_;

  std::vector<scada::DateTimeRange> observed_ranges_;
  std::vector<scada::DateTimeRange> ready_ranges_;

  DataValues values_;

  inline static BoostLogger logger_{LOG_NAME("TimedData")};
};

inline scada::DataValue TimedDataView::GetValueAt(
    const base::Time& time) const {
  auto i = LowerBound(values_, time);

  if (i != values_.size() && values_[i].source_timestamp == time) {
    return values_[i];
  }

  if (i == 0) {
    return {};
  }

  return values_[i - 1];
}

inline void TimedDataView::AddObserver(TimedDataViewObserver& observer,
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

inline void TimedDataView::RemoveObserver(TimedDataViewObserver& observer) {
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

inline void TimedDataView::UpdateObservedRanges() {
  observed_ranges_.clear();

  for (const auto& [observer, range] : observer_ranges_) {
    if (range.first != kTimedDataCurrentOnly) {
      assert(!IsEmptyInterval(range));
      UnionIntervals(observed_ranges_, range);
    }
  }
}

inline std::optional<scada::DateTimeRange> TimedDataView::FindNextGap() const {
  return FindFirstGap(observed_ranges_, ready_ranges_);
}

inline void TimedDataView::AddReadyRange(const scada::DateTimeRange& range) {
  UnionIntervals(ready_ranges_, range);
  NotifyDataReady();
}

inline void TimedDataView::NotifyTimedDataCorrection(
    size_t count,
    const scada::DataValue* tvqs) {
  assert(count > 0);
  assert(IsTimeSorted({tvqs, count}));

  for (auto& o : observers_)
    o.OnTimedDataCorrections(count, tvqs);
}

inline void TimedDataView::NotifyDataReady() {
  for (auto& o : observers_)
    o.OnTimedDataReady();
}

inline bool TimedDataView::InsertOrUpdate(const scada::DataValue& value) {
  ScopedInvariant values_sorted{[&] { return IsTimeSorted(values_); }};

  if (value.source_timestamp.is_null())
    return false;

  // An optimization for tail inserts.
  if (values_.empty() ||
      values_.back().source_timestamp < value.source_timestamp) {
    values_.emplace_back(value);
    return true;
  }

  auto i = LowerBound(values_, value.source_timestamp);
  if (i != values_.size() &&
      values_[i].source_timestamp == value.source_timestamp) {
    if (values_[i].server_timestamp > value.server_timestamp) {
      return false;
    }

    values_[i] = value;
    return true;

  } else {
    values_.insert(values_.begin() + i, value);
    return true;
  }
}

inline void TimedDataView::ClearRange(const scada::DateTimeRange& range) {
  assert(!range.first.is_null());
  assert(range.second.is_null() || range.first <= range.second);

  auto i = LowerBound(values_, range.first);
  auto j = range.second.is_null() ? values_.size()
                                  : UpperBound(values_, range.second);
  values_.erase(values_.begin() + i, values_.begin() + j);
}

inline void TimedDataView::Dump(std::ostream& stream) const {
  stream << "TimedDataView" << std::endl;
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

inline void TimedDataView::ReplaceRange(std::span<scada::DataValue> values) {
  if (values.empty()) {
    return;
  }

  // Optimization: if all new values relate to the same position in history,
  // not overlapping other values, then insert them as whole.
  if (auto i = FindInsertPosition(values_, values.front().source_timestamp,
                                  values.back().source_timestamp)) {
    values_.insert(values_.begin() + *i,
                   std::make_move_iterator(values.begin()),
                   std::make_move_iterator(values.end()));
  } else {
    ReplaceSubrange(values_, {values.data(), values.size()},
                    DataValueTimeLess{});
  }

  assert(IsTimeSorted(values_));
}