#pragma once

#include "base/containers/span.h"
#include "scada/data_value.h"

#include <algorithm>
#include <vector>

using DataValues = std::vector<scada::DataValue>;

struct DataValueTimeLess {
  bool operator()(const scada::DataValue& a, const scada::DataValue& b) const {
    return a.source_timestamp < b.source_timestamp;
  }
};

inline bool IsTimeSorted(base::span<const scada::DataValue> values) {
  // Sorted with no duplicates.
  return std::adjacent_find(values.begin(), values.end(),
                            std::not_fn(DataValueTimeLess{})) == values.end();
}

inline bool IsReverseTimeSorted(base::span<const scada::DataValue> values) {
  // Sorted with no duplicates.
  return std::adjacent_find(values.rbegin(), values.rend(),
                            std::not_fn(DataValueTimeLess{})) == values.rend();
}

inline std::size_t LowerBound(base::span<const scada::DataValue> values,
                              base::Time time) {
  assert(IsTimeSorted(values));
  auto i = std::lower_bound(values.begin(), values.end(), time,
                            [](const scada::DataValue& value, base::Time time) {
                              return value.source_timestamp < time;
                            });
  return i - values.begin();
}

inline std::size_t UpperBound(base::span<const scada::DataValue> values,
                              base::Time time) {
  assert(IsTimeSorted(values));
  auto i = std::upper_bound(values.begin(), values.end(), time,
                            [](base::Time time, const scada::DataValue& value) {
                              return time < value.source_timestamp;
                            });
  return i - values.begin();
}

inline std::size_t ReverseUpperBound(base::span<const scada::DataValue> values,
                                     base::Time time) {
  assert(IsReverseTimeSorted(values));
  auto i = std::upper_bound(values.begin(), values.end(), time,
                            [](base::Time time, const scada::DataValue& value) {
                              return time > value.source_timestamp;
                            });
  return i - values.begin();
}
