#pragma once

#include "base/boost_log.h"
#include "base/constraints.h"
#include "base/format_time.h"
#include "base/observer_list.h"
#include "common/interval_util.h"
#include "timed_data/timed_data_delegate.h"
#include "timed_data/timed_data_dump_util.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_util.h"

class TimedDataView final {
 public:
  ~TimedDataView() { assert(!observers_.might_have_observers()); }

  const DataValues& values() const { return values_; }

  scada::DataValue GetValueAt(const base::Time& time) const;

  // Merge requested data with existing real-time data by collection time.
  void MergeValues(std::span<scada::DataValue> values);

  bool UpdateHistory(const scada::DataValue& value);

  void Clear(const scada::DateTimeRange& range);

  const std::vector<scada::DateTimeRange>& ready_ranges() const {
    return ready_ranges_;
  }

  // Update |ready_from_| to new |time| if new time is earlier.
  void SetReady(const scada::DateTimeRange& range);

  const base::ObserverList<TimedDataDelegate>& observers() const {
    return observers_;
  }

  const std::vector<scada::DateTimeRange>& ranges() const { return ranges_; }

  void AddObserver(TimedDataDelegate& observer,
                   const scada::DateTimeRange& range);
  void RemoveObserver(TimedDataDelegate& observer);

  void RebuildRanges();
  std::optional<scada::DateTimeRange> FindNextGap() const;

  void NotifyTimedDataCorrection(size_t count, const scada::DataValue* tvqs);
  void NotifyPropertyChanged(const PropertySet& properties);
  void NotifyDataReady();
  void NotifyEventsChanged();

  void Dump(std::ostream& stream) const;

 private:
  base::ObserverList<TimedDataDelegate> observers_;
  std::map<TimedDataDelegate*, scada::DateTimeRange> observer_ranges_;

  std::vector<scada::DateTimeRange> ranges_;
  std::vector<scada::DateTimeRange> ready_ranges_;

  DataValues values_;

  inline static BoostLogger logger_{LOG_NAME("TimedData")};
};

inline scada::DataValue TimedDataView::GetValueAt(
    const base::Time& time) const {
  auto i = LowerBound(values_, time);

  if (i != values_.end() && i->source_timestamp == time)
    return *i;

  if (i == values_.begin())
    return {};

  --i;
  if (i == values_.end())
    return {};

  return *i;
}

inline void TimedDataView::AddObserver(TimedDataDelegate& observer,
                                       const scada::DateTimeRange& range) {
  assert(IsValidInterval(range));

  if (!observers_.HasObserver(&observer))
    observers_.AddObserver(&observer);

  bool inserted = observer_ranges_.insert_or_assign(&observer, range).second;

  LOG_INFO(logger_) << "Add observer" << LOG_TAG("New", inserted)
                    << LOG_TAG("From", FormatTime(range.first))
                    << LOG_TAG("To", FormatTime(range.second));

  RebuildRanges();
}

inline void TimedDataView::RemoveObserver(TimedDataDelegate& observer) {
  observers_.RemoveObserver(&observer);

  scada::DateTimeRange range{kTimedDataCurrentOnly, kTimedDataCurrentOnly};
  auto i = observer_ranges_.find(&observer);
  if (i != observer_ranges_.end()) {
    range = i->second;
    observer_ranges_.erase(i);
  }

  LOG_INFO(logger_) << "Remove observer"
                    << LOG_TAG("From", FormatTime(range.first))
                    << LOG_TAG("To", FormatTime(range.second));

  RebuildRanges();
}

inline void TimedDataView::RebuildRanges() {
  ranges_.clear();

  for (const auto& [observer, range] : observer_ranges_) {
    if (range.first != kTimedDataCurrentOnly) {
      UnionIntervals(ranges_, range);
    }
  }
}

inline std::optional<scada::DateTimeRange> TimedDataView::FindNextGap() const {
  return FindFirstGap(ranges_, ready_ranges_);
}

inline void TimedDataView::SetReady(const scada::DateTimeRange& range) {
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

inline void TimedDataView::NotifyPropertyChanged(
    const PropertySet& properties) {
  for (auto& o : observers_)
    o.OnPropertyChanged(properties);
}

inline void TimedDataView::NotifyDataReady() {
  for (auto& o : observers_)
    o.OnTimedDataReady();
}

inline void TimedDataView::NotifyEventsChanged() {
  for (auto& o : observers_)
    o.OnEventsChanged();
}

inline bool TimedDataView::UpdateHistory(const scada::DataValue& value) {
  ScopedInvariant values_sorted{[&] { return IsTimeSorted(values_); }};

  if (value.source_timestamp.is_null())
    return false;

  // An optimization for the back inserts.
  if (values_.empty() ||
      values_.back().source_timestamp < value.source_timestamp) {
    values_.emplace_back(value);
    return true;
  }

  auto i = LowerBound(values_, value.source_timestamp);
  if (i != values_.end() && i->source_timestamp == value.source_timestamp) {
    if (i->server_timestamp > value.server_timestamp)
      return false;

    *i = value;
    return true;

  } else {
    values_.insert(i, value);
    return true;
  }
}

inline void TimedDataView::Clear(const scada::DateTimeRange& range) {
  assert(!range.first.is_null());
  assert(range.second.is_null() || range.first <= range.second);

  auto i = LowerBound(values_, range.first);
  auto j = range.second.is_null() ? values_.end()
                                  : UpperBound(values_, range.second);
  values_.erase(i, j);
}

inline void TimedDataView::Dump(std::ostream& stream) const {
  stream << "TimedDataView" << std::endl;
  stream << "Observers:" << std::endl;
  internal::Dump(stream, observer_ranges_);
  stream << "Requested ranges:" << std::endl;
  internal::Dump(stream, ranges_);
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

inline void TimedDataView::MergeValues(std::span<scada::DataValue> values) {
  if (values.empty()) {
    return;
  }

  // Optimization: if all new values relate to the same position in history,
  // not overlapping other values, then insert them as whole.
  if (auto i = FindInsertPosition(values_, values.front().source_timestamp,
                                  values.back().source_timestamp)) {
    values_.insert(*i, std::make_move_iterator(values.begin()),
                   std::make_move_iterator(values.end()));
  } else {
    ReplaceSubrange(values_, {values.data(), values.size()},
                    DataValueTimeLess{});
  }

  assert(IsTimeSorted(values_));
}