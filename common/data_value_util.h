#pragma once

#include "base/containers/span.h"
#include "core/data_value.h"

#include <algorithm>
#include <vector>

using DataValues = std::vector<scada::DataValue>;

struct DataValueTimeLess {
  bool operator()(const scada::DataValue& a, const scada::DataValue& b) const {
    return std::tie(a.source_timestamp, a.server_timestamp) <
           std::tie(b.source_timestamp, b.server_timestamp);
  }
};

struct DataValueTimeGreater {
  bool operator()(const scada::DataValue& a, const scada::DataValue& b) const {
    return less(b, a);
  }

  DataValueTimeLess less;
};

inline bool IsTimeSorted(base::span<const scada::DataValue> values) {
  return std::is_sorted(values.begin(), values.end(), DataValueTimeLess{});
}

inline bool IsReverseTimeSorted(base::span<const scada::DataValue> values) {
  return std::is_sorted(values.begin(), values.end(), DataValueTimeGreater{});
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

inline DataValues::const_iterator LowerBound(const DataValues& values,
                                             base::Time time) {
  return values.begin() +
         LowerBound(base::span{values.data(), values.size()}, time);
}

inline DataValues::const_iterator UpperBound(const DataValues& values,
                                             base::Time time) {
  return values.begin() +
         UpperBound(base::span{values.data(), values.size()}, time);
}

inline DataValues::iterator LowerBound(DataValues& values, base::Time time) {
  return values.begin() +
         LowerBound(base::span{values.data(), values.size()}, time);
}

inline DataValues::iterator UpperBound(DataValues& values, base::Time time) {
  return values.begin() +
         UpperBound(base::span{values.data(), values.size()}, time);
}
