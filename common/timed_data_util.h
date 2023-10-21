#pragma once

#include "common/data_value_traits.h"
#include "scada/data_value.h"

#include <algorithm>
#include <functional>
#include <span>

// Checks if the `values` are sorted with no duplicates.
template <class T>
inline bool IsTimeSorted(std::span<const T> values) {
  return std::ranges::adjacent_find(values, std::greater_equal{},
                                    &TimedDataTraits<T>::timestamp) ==
         values.end();
}

// An overload for `std::vector`.
template <class T>
inline bool IsTimeSorted(const std::vector<T>& values) {
  return IsTimeSorted(std::span{values});
}

// Checks if the `values` are sorted in reverse order with no duplicates.
template <class T>
inline bool IsReverseTimeSorted(std::span<const T> values) {
  return std::adjacent_find(values.rbegin(), values.rend(),
                            [](const T& a, const T& b) {
                              return TimedDataTraits<T>::timestamp(a) >=
                                     TimedDataTraits<T>::timestamp(b);
                            }) == values.rend();
}

// An overload for `std::vector`.
template <class T>
inline bool IsReverseTimeSorted(const std::vector<T>& values) {
  return IsReverseTimeSorted(std::span{values});
}

template <class T>
inline std::size_t LowerBound(std::span<const T> values,
                              scada::DateTime source_timestamp) {
  assert(IsTimeSorted(values));
  auto i = std::ranges::lower_bound(values, source_timestamp, std::less{},
                                    &TimedDataTraits<T>::timestamp);
  return i - values.begin();
}

// An overload for `std::vector`.
template <class T>
inline std::size_t LowerBound(const std::vector<T>& values,
                              scada::DateTime source_timestamp) {
  return LowerBound(std::span{values}, source_timestamp);
}

template <class T>
inline std::size_t UpperBound(std::span<const T> values,
                              scada::DateTime source_timestamp) {
  assert(IsTimeSorted(values));
  auto i = std::ranges::upper_bound(values, source_timestamp, std::less{},
                                    &TimedDataTraits<T>::timestamp);
  return i - values.begin();
}

// An overload for `std::vector`.
template <class T>
inline std::size_t UpperBound(const std::vector<T>& values,
                              scada::DateTime source_timestamp) {
  return UpperBound(std::span{values}, source_timestamp);
}

template <class T>
inline std::size_t ReverseUpperBound(std::span<const T> values,
                                     scada::DateTime source_timestamp) {
  assert(IsReverseTimeSorted(values));
  auto i = std::ranges::upper_bound(values, source_timestamp, std::greater{},
                                    &TimedDataTraits<T>::timestamp);
  return i - values.begin();
}

// An overload for `std::vector`.
template <class T>
inline std::size_t ReverseUpperBound(const std::vector<T>& values,
                                     scada::DateTime source_timestamp) {
  return ReverseUpperBound(std::span{values}, source_timestamp);
}