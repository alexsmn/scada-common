#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

template <class T>
using Interval = std::pair<T, T>;

template <class T>
inline bool IsValidInterval(const Interval<T>& interval) {
  return interval.first <= interval.second;
}

template <class T>
inline bool IsEmptyInterval(const Interval<T>& interval) {
  assert(IsValidInterval(interval));
  return interval.first == interval.second;
}

template <class T>
inline bool IsValidInterval(const std::vector<Interval<T>>& intervals) {
  return std::all_of(
             intervals.begin(), intervals.end(),
             [](const auto& range) { return IsValidInterval(range); }) &&
         std::is_sorted(intervals.begin(), intervals.end());
}

template <class T>
inline bool IntervalContains(const Interval<T>& a, const Interval<T>& b) {
  assert(IsValidInterval(a));
  assert(IsValidInterval(b));
  return a.first <= b.first && b.second <= a.second;
}

template <class T>
inline bool IntervalContains(const std::vector<Interval<T>>& intervals,
                             const Interval<T>& interval) {
  assert(IsValidInterval(intervals));
  assert(IsValidInterval(interval));

  auto i = std::lower_bound(intervals.begin(), intervals.end(), interval.first,
                            [](const auto& interval, const auto& bound) {
                              return interval.second < bound;
                            });
  if (i == intervals.end())
    return false;

  auto& found_interval = *i;
  return IntervalContains(found_interval, interval);
}

template <class T>
inline bool IntervalsOverlap(const Interval<T>& a, const Interval<T>& b) {
  assert(IsValidInterval(a));
  assert(IsValidInterval(b));
  return a.first < b.second && b.first < a.second;
}

template <class T>
inline Interval<T> IntervalsIntersection(const Interval<T>& a,
                                         const Interval<T>& b) {
  assert(IntervalsOverlap(a, b));
  return {std::max(a.first, b.first), std::min(a.second, b.second)};
}

template <class T>
inline Interval<T> UnionIntervals(const Interval<T>& a, const Interval<T>& b) {
  assert(IsValidInterval(a));
  assert(IsValidInterval(b));
  return {std::min(a.first, b.first), std::max(a.second, b.second)};
}

template <class T>
inline bool UnionIntervals(std::vector<Interval<T>>& intervals,
                           const Interval<T>& interval) {
  assert(IsValidInterval(intervals));
  assert(IsValidInterval(interval));

  // Find the position to remove from. It's the last interval that ends just
  // before the new interval start.

  auto i = std::lower_bound(intervals.begin(), intervals.end(), interval.first,
                            [](const auto& interval, const auto& bound) {
                              return interval.second < bound;
                            });

  // If the new interval is in the end or it doesn't intersect anything, then
  // just insert the new interval.
  if (i == intervals.end() || interval.second < i->first) {
    intervals.insert(i, interval);
    return true;
  }

  // If the interval is fully contained, then ignore it.
  if (i->first <= interval.first && interval.second <= i->second)
    return false;

  // Find the position to remove until. It's the interval that starts after or
  // exactly on the new interval end. The actual position is after this
  // interval.

  auto j = std::upper_bound(i, intervals.end(), interval.second,
                            [](const auto& bound, const auto& interval) {
                              return bound < interval.first;
                            });

  // Update the first interval and remove others.

  i->first = std::min(i->first, interval.first);
  i->second = std::max(std::prev(j)->second, interval.second);

  assert(i != j);
  intervals.erase(std::next(i), j);

  assert(std::is_sorted(intervals.begin(), intervals.end()));
  return true;
}
