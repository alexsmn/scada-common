#pragma once

#include "scada/data_value.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <span>

template <typename T>
struct TimedDataTraits;

// Checks if the `values` are sorted with no duplicates.
template <class T>
inline bool IsTimeSorted(std::span<T> values) {
  return std::ranges::adjacent_find(
             values, std::greater_equal{},
             &TimedDataTraits<std::decay_t<T>>::timestamp) == values.end();
}

// An overload for `std::vector`.
template <class T>
inline bool IsTimeSorted(const std::vector<T>& values) {
  return IsTimeSorted(std::span{values});
}

// Checks if the `values` are sorted in reverse order with no duplicates.
template <class T>
inline bool IsReverseTimeSorted(std::span<T> values) {
  return std::adjacent_find(
             values.rbegin(), values.rend(), [](const T& a, const T& b) {
               return TimedDataTraits<std::decay_t<T>>::timestamp(a) >=
                      TimedDataTraits<std::decay_t<T>>::timestamp(b);
             }) == values.rend();
}

// An overload for `std::vector`.
template <class T>
inline bool IsReverseTimeSorted(const std::vector<T>& values) {
  return IsReverseTimeSorted(std::span{values});
}

template <class T>
inline std::size_t LowerBound(std::span<T> values, scada::DateTime time) {
  assert(IsTimeSorted(values));
  auto i = std::ranges::lower_bound(
      values, time, std::less{}, &TimedDataTraits<std::decay_t<T>>::timestamp);
  return i - values.begin();
}

// An overload for `std::vector`.
template <class T>
inline std::size_t LowerBound(const std::vector<T>& values,
                              scada::DateTime time) {
  return LowerBound(std::span{values}, time);
}

template <class T>
inline std::size_t UpperBound(std::span<T> values, scada::DateTime time) {
  assert(IsTimeSorted(values));
  auto i = std::ranges::upper_bound(
      values, time, std::less{}, &TimedDataTraits<std::decay_t<T>>::timestamp);
  return i - values.begin();
}

// An overload for `std::vector`.
template <class T>
inline std::size_t UpperBound(const std::vector<T>& values,
                              scada::DateTime time) {
  return UpperBound(std::span{values}, time);
}

template <class T>
inline T* GetValueAt(std::span<T> values, scada::DateTime time) {
  auto i = LowerBound(values, time);

  if (i != values.size() &&
      TimedDataTraits<std::decay_t<T>>::timestamp(values[i]) == time) {
    return &values[i];
  }

  if (i == 0) {
    return nullptr;
  }

  return &values[i - 1];
}

template <class T>
inline std::size_t ReverseUpperBound(std::span<const T> values,
                                     scada::DateTime time) {
  assert(IsReverseTimeSorted(values));
  auto i = std::ranges::upper_bound(values, time, std::greater{},
                                    &TimedDataTraits<T>::timestamp);
  return i - values.begin();
}

// An overload for `std::vector`.
template <class T>
inline std::size_t ReverseUpperBound(const std::vector<T>& values,
                                     scada::DateTime time) {
  return ReverseUpperBound(std::span{values}, time);
}

template <class T>
inline std::optional<size_t> FindInsertPosition(std::span<const T> values,
                                                scada::DateTime from,
                                                scada::DateTime to) {
  auto i = LowerBound(values, from);
  if (i != values.size() && TimedDataTraits<T>::timestamp(values[i]) == from) {
    return std::nullopt;
  }

  auto j = LowerBound(values, to);
  if (j != values.size() && TimedDataTraits<T>::timestamp(values[j]) == to) {
    return std::nullopt;
  }

  if (i != j) {
    return std::nullopt;
  }

  return i;
}

// An overload for `std::vector`.
template <class T>
inline std::optional<size_t> FindInsertPosition(const std::vector<T>& values,
                                                scada::DateTime from,
                                                scada::DateTime to) {
  return FindInsertPosition(std::span{values}, from, to);
}

template <class T, class Compare>
inline void ReplaceSubrange(std::vector<T>& values,
                            std::span<T> updates,
                            Compare comp) {
  assert(std::is_sorted(values.begin(), values.end(), comp));
  assert(std::is_sorted(updates.begin(), updates.end(), comp));

  if (updates.empty())
    return;

  // TODO: Switch to `LowerBound`.
  auto first = std::lower_bound(values.begin(), values.end(), updates[0], comp);
  auto last = std::upper_bound(values.begin(), values.end(),
                               updates[updates.size() - 1], comp);
  auto count = static_cast<size_t>(std::distance(first, last));

  auto copy_count = std::min<size_t>(count, updates.size());
  std::copy(std::make_move_iterator(updates.begin()),
            std::make_move_iterator(updates.begin() + copy_count), first);

  if (count < updates.size()) {
    values.insert(last, std::make_move_iterator(updates.begin() + copy_count),
                  std::make_move_iterator(updates.end()));
  } else {
    values.erase(first + copy_count, last);
  }

  assert(std::is_sorted(values.begin(), values.end(), comp));
}
