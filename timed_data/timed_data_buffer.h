#pragma once

#include "base/boost_log.h"
#include "base/check.h"
#include "base/constraints.h"
#include "base/format_time.h"
#include "base/interval_util.h"
#include "base/lifetime.h"
#include "base/observer_list.h"
#include "common/data_value_traits.h"
#include "common/timed_data_util.h"
#include "timed_data/timed_data_buffer_fwd.h"
#include "timed_data/timed_data_util.h"
#include "timed_data/timed_data_view.h"
#include "timed_data/timed_data_view_dump.h"
#include "timed_data/timed_data_view_observer.h"

#include <algorithm>
#include <limits>
#include <optional>

// Retention policy for a BasicTimedDataBuffer. Bounds memory so history that no
// observer is looking at is not kept forever.
struct RetentionPolicy {
  // Keep only samples inside the covering hull of the observed ranges (plus the
  // current value, which BaseTimedData maintains separately). When no observer
  // requests history, all history is dropped.
  bool observed_window = true;

  // Backstop high-water mark. When the sample count exceeds this, the oldest
  // samples are dropped. `max()` disables the cap.
  size_t max_samples = std::numeric_limits<size_t>::max();
};

// Owning, sorted time-series container. Holds a run of samples sorted by
// timestamp, tracks which time ranges observers care about ("observed") and
// which have been filled ("ready"), fans out mutations to observers, and evicts
// samples outside the observed working set to bound memory.
//
// Non-copyable and non-movable: it is owned in place by BaseTimedData and
// referenced by the history fetcher, so its address must be stable.
//
// Read access is handed out as a lightweight BasicTimedDataView (see view()),
// which is the supported way to slice and point-query the samples.
//
// Mutations (InsertOrUpdate/ReplaceRange/ClearRange) notify observers of the
// affected time range automatically. To apply several mutations as one logical
// change and emit a single coalesced notification, hold a BeginUpdate() scope
// around them.
template <typename T>
class BasicTimedDataBuffer final {
 public:
  BasicTimedDataBuffer() = default;
  ~BasicTimedDataBuffer() { base::Check(!observers_.might_have_observers()); }

  BasicTimedDataBuffer(const BasicTimedDataBuffer&) = delete;
  BasicTimedDataBuffer& operator=(const BasicTimedDataBuffer&) = delete;

  using ObservedRangesUpdatedHandler = std::function<void(bool has_observers)>;

  void set_observed_ranges_updated_handler(
      ObservedRangesUpdatedHandler handler) {
    observed_ranges_updated_handler_ = std::move(handler);
  }

  void set_retention(RetentionPolicy retention) {
    retention_ = retention;
    TrimToObservedRanges();
  }

  // Coalesces the mutations made while it is alive into a single observer
  // notification emitted when the outermost scope ends. Nesting is ref-counted.
  class [[nodiscard]] ScopedUpdateBatch {
   public:
    ScopedUpdateBatch() = default;
    explicit ScopedUpdateBatch(BasicTimedDataBuffer* buffer) : buffer_{buffer} {
      if (buffer_)
        ++buffer_->batch_depth_;
    }
    ScopedUpdateBatch(ScopedUpdateBatch&& other) noexcept
        : buffer_{other.buffer_} {
      other.buffer_ = nullptr;
    }
    ScopedUpdateBatch& operator=(ScopedUpdateBatch&& other) noexcept {
      if (this != &other) {
        Close();
        buffer_ = other.buffer_;
        other.buffer_ = nullptr;
      }
      return *this;
    }
    ~ScopedUpdateBatch() { Close(); }

   private:
    void Close() {
      if (buffer_) {
        buffer_->EndBatch();
        buffer_ = nullptr;
      }
    }
    BasicTimedDataBuffer* buffer_ = nullptr;
  };

  ScopedUpdateBatch BeginUpdate() SCADA_LIFETIME_BOUND {
    return ScopedUpdateBatch{this};
  }

  // Read access.

  // A non-owning view over the whole sorted run. Cheap to copy and slice.
  // The view borrows this buffer's storage; it dangles across any mutation that
  // reallocates the storage, so re-fetch it after mutating rather than caching.
  BasicTimedDataView<T> view() const SCADA_LIFETIME_BOUND {
    return BasicTimedDataView<T>{std::span<const T>{values_}};
  }
  BasicTimedDataView<T> view(const scada::DateTimeRange& range) const
      SCADA_LIFETIME_BOUND {
    return view().slice(range);
  }

  const T& at(size_t index) const SCADA_LIFETIME_BOUND { return values_[index]; }

  const std::vector<T>& values() const SCADA_LIFETIME_BOUND { return values_; }

  // Returns the sample at or before `time`, or null when `time` precedes all
  // samples. Returns a pointer instead of an optional for performance reasons.
  const T* GetValueAt(scada::DateTime time) const SCADA_LIFETIME_BOUND {
    return view().sample_at_or_before(time);
  }

  // Mutation. Each mutator notifies observers of the affected time range (or
  // contributes to the enclosing BeginUpdate() scope's coalesced notification).

  // Inserts `value`, or overwrites an existing sample with the same timestamp
  // when `value` sorts later (see TimedDataTraits::IsLesser). Returns false if
  // no change was applied.
  bool InsertOrUpdate(const T& value);

  // Merges `values` into the buffer, replacing anything already present in the
  // overlapped time range. `values` is sanitized (nulls dropped, sorted,
  // de-duplicated) so callers need not pre-clean history batches.
  void ReplaceRange(std::span<T> values);

  // Drops all samples inside `range`.
  void ClearRange(const scada::DateTimeRange& range);

  // Observation.

  void AddObserver(BasicTimedDataViewObserver<T>& observer,
                   const scada::DateTimeRange& range);
  void RemoveObserver(BasicTimedDataViewObserver<T>& observer);

  // Readiness.

  const std::vector<scada::DateTimeRange>& ready_ranges() const
      SCADA_LIFETIME_BOUND {
    return ready_ranges_;
  }

  void AddReadyRange(const scada::DateTimeRange& range);

  // Finds a next observed range that's not covered by a ready range.
  std::optional<scada::DateTimeRange> FindNextGap() const;

  // Rest.

  void Dump(std::ostream& stream) const;

 private:
  void UpdateObservedRanges();

  // Records that samples in `range` changed. Notifies immediately when no
  // BeginUpdate() scope is active, otherwise coalesces into that scope.
  void MarkDirty(const scada::DateTimeRange& range);

  void EndBatch();

  // Notifies observers of the samples currently in `range`. No-op when empty.
  void NotifyRange(const scada::DateTimeRange& range) {
    BasicTimedDataView<T> slice = view(range);
    if (!slice.empty()) {
      for (auto& o : observers_)
        o.OnTimedDataUpdates(slice.samples());
    }
  }

  // Evicts samples outside the retained working set (covering hull of the
  // observed ranges, bounded by the max-samples backstop) and clamps
  // `ready_ranges_` to match so gap-finding stays consistent.
  void TrimToObservedRanges();

  // Clamps each interval in `ranges` to [lo, hi] (a null `hi` means no upper
  // bound), dropping intervals that become empty. `ranges` stays sorted.
  static void ClampRanges(std::vector<scada::DateTimeRange>& ranges,
                          scada::DateTime lo,
                          scada::DateTime hi);

  static scada::DateTimeRange UnionRange(const scada::DateTimeRange& a,
                                         const scada::DateTimeRange& b);

  // For convenience.
  static constexpr scada::DateTime timestamp(const T& value) {
    return TimedDataTraits<T>::timestamp(value);
  }

  base::ObserverList<BasicTimedDataViewObserver<T>> observers_;
  std::map<BasicTimedDataViewObserver<T>*, scada::DateTimeRange> observer_ranges_;

  std::vector<scada::DateTimeRange> observed_ranges_;
  std::vector<scada::DateTimeRange> ready_ranges_;

  ObservedRangesUpdatedHandler observed_ranges_updated_handler_;

  RetentionPolicy retention_;

  int batch_depth_ = 0;
  std::optional<scada::DateTimeRange> dirty_;

  std::vector<T> values_;

  inline static BoostLogger logger_{LOG_NAME("TimedData")};
};

template <typename T>
inline void BasicTimedDataBuffer<T>::AddObserver(
    BasicTimedDataViewObserver<T>& observer,
    const scada::DateTimeRange& range) {
  base::Check(!range.second.is_null());
  base::Check(IsValidInterval(range));
  base::Check(range.first == kTimedDataCurrentOnly || !IsEmptyInterval(range));

  if (!observers_.HasObserver(&observer))
    observers_.AddObserver(&observer);

  bool inserted = observer_ranges_.insert_or_assign(&observer, range).second;

  LOG_INFO(logger_) << "Add observer" << LOG_TAG("New", inserted)
                    << LOG_TAG("From", FormatTime(range.first))
                    << LOG_TAG("To", FormatTime(range.second));

  UpdateObservedRanges();
}

template <typename T>
inline void BasicTimedDataBuffer<T>::RemoveObserver(
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
inline void BasicTimedDataBuffer<T>::UpdateObservedRanges() {
  observed_ranges_.clear();

  for (const auto& [observer, range] : observer_ranges_) {
    if (range.first != kTimedDataCurrentOnly) {
      base::Check(!IsEmptyInterval(range));
      UnionIntervals(observed_ranges_, range);
    }
  }

  // An observer narrowing or dropping its range can shrink the working set.
  TrimToObservedRanges();

  if (observed_ranges_updated_handler_) {
    observed_ranges_updated_handler_(!observed_ranges_.empty());
  }
}

template <typename T>
inline std::optional<scada::DateTimeRange>
BasicTimedDataBuffer<T>::FindNextGap() const {
  return FindFirstGap(observed_ranges_, ready_ranges_);
}

template <typename T>
inline void BasicTimedDataBuffer<T>::AddReadyRange(
    const scada::DateTimeRange& range) {
  UnionIntervals(ready_ranges_, range);

  for (auto& o : observers_) {
    o.OnTimedDataReady();
  }
}

template <typename T>
inline void BasicTimedDataBuffer<T>::MarkDirty(
    const scada::DateTimeRange& range) {
  if (batch_depth_ > 0) {
    dirty_ = dirty_ ? UnionRange(*dirty_, range) : range;
    return;
  }
  NotifyRange(range);
}

template <typename T>
inline void BasicTimedDataBuffer<T>::EndBatch() {
  base::Check(batch_depth_ > 0);
  if (--batch_depth_ == 0 && dirty_) {
    scada::DateTimeRange range = *dirty_;
    dirty_.reset();
    NotifyRange(range);
  }
}

template <typename T>
inline bool BasicTimedDataBuffer<T>::InsertOrUpdate(const T& value) {
  ScopedInvariant values_sorted{[&] { return IsTimeSorted(values_); }};

  if (timestamp(value).is_null())
    return false;

  // An optimization for tail inserts.
  if (values_.empty() || timestamp(values_.back()) < timestamp(value)) {
    values_.emplace_back(value);
  } else {
    auto i = LowerBound(values_, timestamp(value));
    if (i != values_.size() && timestamp(values_[i]) == timestamp(value)) {
      if (TimedDataTraits<T>::IsLesser(value, values_[i])) {
        return false;
      }
      values_[i] = value;
    } else {
      values_.insert(values_.begin() + i, value);
    }
  }

  MarkDirty({timestamp(value), timestamp(value)});
  return true;
}

template <typename T>
inline void BasicTimedDataBuffer<T>::ClearRange(
    const scada::DateTimeRange& range) {
  base::Check(!range.first.is_null());
  base::Check(range.second.is_null() || range.first <= range.second);

  auto i = LowerBound(values_, range.first);
  auto j = range.second.is_null() ? values_.size()
                                  : UpperBound(values_, range.second);
  if (i == j)
    return;
  values_.erase(values_.begin() + i, values_.begin() + j);

  // Observers re-read the affected window; the coalesced range covers it if a
  // re-populating mutation follows within the same BeginUpdate() scope.
  MarkDirty(range);
}

template <typename T>
inline void BasicTimedDataBuffer<T>::Dump(std::ostream& stream) const {
  stream << "BasicTimedDataBuffer" << std::endl;
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
inline void BasicTimedDataBuffer<T>::ReplaceRange(std::span<T> values) {
  // Sanitize: drop samples without a timestamp, sort, and de-duplicate by
  // timestamp so callers can hand us a raw history batch. This keeps the
  // sortedness invariant satisfied by construction rather than by discipline.
  // A null timestamp sorts as the minimum, so if any is present it is at the
  // front of an otherwise-sorted batch; check that too, since IsTimeSorted
  // alone would accept a single leading null.
  std::vector<T> sanitized;
  const bool has_leading_null =
      !values.empty() && timestamp(values.front()).is_null();
  if (has_leading_null || !IsTimeSorted(values)) {
    sanitized.assign(std::make_move_iterator(values.begin()),
                     std::make_move_iterator(values.end()));
    std::erase_if(sanitized,
                  [](const T& v) { return timestamp(v).is_null(); });
    std::ranges::stable_sort(sanitized, std::less{},
                             &TimedDataTraits<T>::timestamp);
    auto dup = std::ranges::unique(sanitized, std::equal_to{},
                                   &TimedDataTraits<T>::timestamp);
    sanitized.erase(dup.begin(), dup.end());
    values = std::span<T>{sanitized};
  }

  if (values.empty()) {
    return;
  }

  scada::DateTimeRange changed{timestamp(values.front()),
                               timestamp(values.back())};

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

  base::Check(IsTimeSorted(values_));

  // A landed batch can push us over the working set; trim before notifying so
  // observers see the retained window.
  TrimToObservedRanges();

  MarkDirty(changed);
}

template <typename T>
inline void BasicTimedDataBuffer<T>::TrimToObservedRanges() {
  if (values_.empty())
    return;

  if (retention_.observed_window) {
    if (observed_ranges_.empty()) {
      // No observer wants history: drop all of it. BaseTimedData keeps the
      // current value separately.
      values_.clear();
      values_.shrink_to_fit();
      ready_ranges_.clear();
      return;
    }

    // Keep the covering hull [earliest observed start, latest observed end].
    const scada::DateTime hull_first = observed_ranges_.front().first;
    const scada::DateTime hull_last = observed_ranges_.back().second;

    size_t keep_begin = LowerBound(values_, hull_first);
    size_t keep_end = hull_last.is_null() ? values_.size()
                                          : UpperBound(values_, hull_last);
    if (keep_end < values_.size())
      values_.erase(values_.begin() + keep_end, values_.end());
    if (keep_begin > 0)
      values_.erase(values_.begin(), values_.begin() + keep_begin);

    // Keep ready bookkeeping consistent with retained data so FindNextGap does
    // not re-request just-trimmed ranges.
    ClampRanges(ready_ranges_, hull_first, hull_last);
  }

  // Backstop: bound the absolute sample count by dropping the oldest.
  if (values_.size() > retention_.max_samples) {
    size_t drop = values_.size() - retention_.max_samples;
    scada::DateTime new_first = timestamp(values_[drop]);
    values_.erase(values_.begin(), values_.begin() + drop);
    ClampRanges(ready_ranges_, new_first, scada::DateTime{});
  }

  // Return memory only after a substantial trim, so normal batch-by-batch
  // growth does not reallocate on every ReplaceRange.
  if (values_.capacity() > values_.size() * 2)
    values_.shrink_to_fit();
}

template <typename T>
inline void BasicTimedDataBuffer<T>::ClampRanges(
    std::vector<scada::DateTimeRange>& ranges,
    scada::DateTime lo,
    scada::DateTime hi) {
  std::vector<scada::DateTimeRange> result;
  result.reserve(ranges.size());
  for (const auto& r : ranges) {
    scada::DateTime a = std::max(r.first, lo);
    scada::DateTime b = hi.is_null() ? r.second : std::min(r.second, hi);
    if (a < b)
      result.push_back({a, b});
  }
  ranges = std::move(result);
}

template <typename T>
inline scada::DateTimeRange BasicTimedDataBuffer<T>::UnionRange(
    const scada::DateTimeRange& a,
    const scada::DateTimeRange& b) {
  scada::DateTime first = std::min(a.first, b.first);
  scada::DateTime second = (a.second.is_null() || b.second.is_null())
                               ? scada::DateTime{}
                               : std::max(a.second, b.second);
  return {first, second};
}
