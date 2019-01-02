#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

template <class T>
inline bool IntervalsOverlap(const std::pair<T, T>& a,
                             const std::pair<T, T>& b) {
  return a.first < b.second && b.first < a.second;
}

template <class T>
inline bool UnionIntervals(std::vector<std::pair<T, T>>& ranges,
                           const std::pair<T, T>& range) {
  assert(std::is_sorted(ranges.begin(), ranges.end()));
  assert(range.first <= range.second);

  // Find the position to remove from. It's the last range that ends just before
  // the new range start.

  auto i = std::lower_bound(ranges.begin(), ranges.end(), range.first,
                            [](const auto& range, const auto& bound) {
                              return range.second < bound;
                            });

  // If the new range is in the end or it doesn't intersect anything, then
  // just insert the new range.
  if (i == ranges.end() || range.second < i->first) {
    ranges.insert(i, range);
    return true;
  }

  // If the range is fully contained, then ignore it.
  if (i->first <= range.first && range.second <= i->second)
    return false;

  // Find the position to remove until. It's the range that starts after or
  // exactly on the new range end. The actual position is after this range.

  auto j = std::upper_bound(
      i, ranges.end(), range.second,
      [](const auto& bound, const auto& range) { return bound < range.first; });

  // Update the first range and remove others.

  i->first = std::min(i->first, range.first);
  i->second = std::max(std::prev(j)->second, range.second);

  assert(i != j);
  ranges.erase(std::next(i), j);

  assert(std::is_sorted(ranges.begin(), ranges.end()));
  return true;
}
