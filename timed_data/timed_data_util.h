// TODO: Move everything to `common/timed_data_util.h`.

#pragma once

#include "base/interval_util.h"
#include "timed_data/timed_data.h"

#include <algorithm>
#include <optional>
#include <span>

inline scada::DateTime GetReadyFrom(
    std::span<const scada::DateTimeRange> ready_ranges,
    const scada::DateTimeRange& range) {
  auto i =
      std::lower_bound(ready_ranges.begin(), ready_ranges.end(), range.second,
                       [](const scada::DateTimeRange& range,
                          scada::DateTime time) { return range.first < time; });
  if (i == ready_ranges.end())
    return kTimedDataCurrentOnly;

  if (i->second <= range.first)
    return kTimedDataCurrentOnly;

  return std::max(range.first, i->first);
}

template <class T>
inline std::optional<Interval<T>> FindFirstGap(
    const std::vector<Interval<T>>& ranges,
    const std::vector<Interval<T>>& ready_ranges) {
  assert(AreValidIntervals(ranges));
  assert(AreValidIntervals(ready_ranges));
  auto j = ready_ranges.begin();
  for (auto i = ranges.begin(); i != ranges.end(); ++i) {
    while (j != ready_ranges.end() && j->second <= i->first)
      ++j;
    if (j == ready_ranges.end())
      return *i;
    if (j->first > i->first)
      return Interval<T>{i->first, std::min(i->second, j->first)};
    if (j->second < i->second) {
      const auto& gap_start = j->second;
      auto gap_end = i->second;
      // Till the previous ready range.
      ++j;
      if (j != ready_ranges.end() && j->first < gap_end)
        gap_end = j->first;
      return Interval<T>{gap_start, gap_end};
    }
  }
  return std::nullopt;
}

template <class T>
inline std::optional<Interval<T>> FindLastGap(
    const std::vector<Interval<T>>& ranges,
    const std::vector<Interval<T>>& ready_ranges) {
  assert(AreValidIntervals(ranges));
  assert(AreValidIntervals(ready_ranges));
  auto j = ready_ranges.rbegin();
  for (auto i = ranges.rbegin(); i != ranges.rend(); ++i) {
    while (j != ready_ranges.rend() && j->first >= i->second)
      ++j;
    if (j == ready_ranges.rend())
      return *i;
    if (j->second < i->second)
      return Interval<T>{std::max(j->second, i->first), i->second};
    if (j->first > i->first) {
      auto gap_start = i->first;
      const auto& gap_end = j->first;
      // Till the previous ready range.
      ++j;
      if (j != ready_ranges.rend() && j->second > gap_start)
        gap_start = j->second;
      return Interval<T>{gap_start, gap_end};
    }
  }
  return std::nullopt;
}
