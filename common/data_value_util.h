#pragma once

#include "scada/data_value.h"

#include <algorithm>
#include <functional>
#include <span>

struct DataValueTimeLess {
  bool operator()(const scada::DataValue& a, const scada::DataValue& b) const {
    return a.source_timestamp < b.source_timestamp;
  }
};

inline scada::DateTime GetSourceTimestamp(const scada::DataValue& data_value) {
  return data_value.source_timestamp;
}

// Checks if the `values` are sorted with no duplicates.
inline bool IsTimeSorted(std::span<const scada::DataValue> values) {
  return std::ranges::adjacent_find(values, std::greater_equal{},
                                    &GetSourceTimestamp) == values.end();
}

// Checks if the `values` are sorted in reverse order with no duplicates.
inline bool IsReverseTimeSorted(std::span<const scada::DataValue> values) {
  return std::adjacent_find(values.rbegin(), values.rend(),
                            std::not_fn(DataValueTimeLess{})) == values.rend();
}

inline std::size_t LowerBound(std::span<const scada::DataValue> values,
                              scada::DateTime source_timestamp) {
  assert(IsTimeSorted(values));
  auto i = std::ranges::lower_bound(values, source_timestamp, std::less{},
                                    &GetSourceTimestamp);
  return i - values.begin();
}

inline std::size_t UpperBound(std::span<const scada::DataValue> values,
                              scada::DateTime source_timestamp) {
  assert(IsTimeSorted(values));
  auto i = std::ranges::upper_bound(values, source_timestamp, std::less{},
                                    &GetSourceTimestamp);
  return i - values.begin();
}

inline std::size_t ReverseUpperBound(std::span<const scada::DataValue> values,
                                     scada::DateTime source_timestamp) {
  assert(IsReverseTimeSorted(values));
  auto i = std::ranges::upper_bound(values, source_timestamp, std::greater{},
                                    &GetSourceTimestamp);
  return i - values.begin();
}
