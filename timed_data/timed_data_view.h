#pragma once

#include "base/lifetime.h"
#include "common/data_value_traits.h"
#include "common/timed_data_util.h"
#include "scada/date_time.h"
#include "scada/date_time_range.h"
#include "timed_data/timed_data_view_fwd.h"

#include <span>

// A non-owning, copyable view over a time-sorted run of samples. Cheap to copy
// and to slice (each slice is an O(log n) binary search, never a copy).
//
// The view borrows its samples; the underlying storage must stay alive and
// sorted by TimedDataTraits<T>::timestamp for the view's lifetime. Treat a view
// as transient: re-fetch it after any mutation of the source buffer rather than
// caching it, since a reallocating mutation invalidates the borrowed span.
template <typename T>
class BasicTimedDataView {
 public:
  BasicTimedDataView() = default;
  explicit BasicTimedDataView(std::span<const T> samples SCADA_LIFETIME_BOUND)
      : samples_{samples} {}

  // Slicing. All return sub-views over the same storage; nothing is copied.

  // The samples whose timestamp falls in `range` ([first, second], with a null
  // `second` meaning "to the end").
  BasicTimedDataView slice(const scada::DateTimeRange& range) const
      SCADA_LIFETIME_BOUND {
    return from(range.first).until(range.second);
  }

  // The samples whose timestamp is >= `start` (no lower bound if `start` null).
  BasicTimedDataView from(scada::DateTime start) const SCADA_LIFETIME_BOUND {
    if (start.is_null())
      return *this;
    size_t i = LowerBound(samples_, start);
    return BasicTimedDataView{samples_.subspan(i)};
  }

  // The samples whose timestamp is <= `end` (no upper bound if `end` null).
  BasicTimedDataView until(scada::DateTime end) const SCADA_LIFETIME_BOUND {
    if (end.is_null())
      return *this;
    size_t i = UpperBound(samples_, end);
    return BasicTimedDataView{samples_.subspan(0, i)};
  }

  // Sequential access.

  std::span<const T> samples() const SCADA_LIFETIME_BOUND { return samples_; }
  size_t size() const { return samples_.size(); }
  bool empty() const { return samples_.empty(); }
  const T& operator[](size_t i) const SCADA_LIFETIME_BOUND {
    return samples_[i];
  }
  const T& front() const SCADA_LIFETIME_BOUND { return samples_.front(); }
  const T& back() const SCADA_LIFETIME_BOUND { return samples_.back(); }
  auto begin() const SCADA_LIFETIME_BOUND { return samples_.begin(); }
  auto end() const SCADA_LIFETIME_BOUND { return samples_.end(); }

  // Point lookup. Returns a pointer rather than an optional so a hit costs no
  // copy; null means "no such sample".

  // The sample whose timestamp equals `time` exactly, or null.
  const T* value_at(scada::DateTime time) const SCADA_LIFETIME_BOUND {
    size_t i = LowerBound(samples_, time);
    if (i != samples_.size() &&
        TimedDataTraits<T>::timestamp(samples_[i]) == time) {
      return &samples_[i];
    }
    return nullptr;
  }

  // The last sample whose timestamp is <= `time`, or null when `time` precedes
  // all samples. This is the honest name for the old GetValueAt lower-bound
  // behavior.
  const T* sample_at_or_before(scada::DateTime time) const
      SCADA_LIFETIME_BOUND {
    return GetValueAt(samples_, time);
  }

  // The [first timestamp, last timestamp] span, or an empty range when empty.
  scada::DateTimeRange time_span() const {
    if (samples_.empty())
      return {};
    return {TimedDataTraits<T>::timestamp(samples_.front()),
            TimedDataTraits<T>::timestamp(samples_.back())};
  }

 private:
  std::span<const T> samples_;
};
