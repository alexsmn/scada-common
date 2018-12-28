#pragma once

#include "base/span.h"
#include "core/data_value.h"

#include <algorithm>
#include <vector>

using DataValues = std::vector<scada::DataValue>;

struct DataValueTimeLess {
  bool operator()(const scada::DataValue& a, const scada::DataValue& b) const {
    return a.source_timestamp < b.source_timestamp;
  }
};

struct DataValueTimeGreater {
  bool operator()(const scada::DataValue& a, const scada::DataValue& b) const {
    return a.source_timestamp > b.source_timestamp;
  }
};

inline bool IsTimeSorted(span<const scada::DataValue> values) {
  return std::is_sorted(values.begin(), values.end(), DataValueTimeLess{});
}

inline bool IsReverseTimeSorted(span<const scada::DataValue> values) {
  return std::is_sorted(values.begin(), values.end(), DataValueTimeGreater{});
}

inline std::size_t LowerBound(span<const scada::DataValue> values,
                              base::Time time) {
  assert(IsTimeSorted(values));
  assert(!time.is_null());
  auto i = std::lower_bound(values.begin(), values.end(), time,
                            [](const scada::DataValue& value, base::Time time) {
                              return value.source_timestamp < time;
                            });
  return i - values.begin();
}

inline std::size_t UpperBound(span<const scada::DataValue> values,
                              base::Time time) {
  assert(IsTimeSorted(values));
  assert(!time.is_null());
  auto i = std::upper_bound(values.begin(), values.end(), time,
                            [](base::Time time, const scada::DataValue& value) {
                              return time < value.source_timestamp;
                            });
  return i - values.begin();
}

inline std::size_t ReverseUpperBound(span<const scada::DataValue> values,
                                     base::Time time) {
  assert(!time.is_null());
  assert(IsReverseTimeSorted(values));
  auto i = std::upper_bound(values.begin(), values.end(), time,
                            [](base::Time time, const scada::DataValue& value) {
                              return time > value.source_timestamp;
                            });
  return i - values.begin();
}

inline DataValues::const_iterator LowerBound(const DataValues& values,
                                             base::Time time) {
  return values.begin() + LowerBound(span{values.data(), values.size()}, time);
}

inline DataValues::const_iterator UpperBound(const DataValues& values,
                                             base::Time time) {
  return values.begin() + UpperBound(span{values.data(), values.size()}, time);
}

inline DataValues::iterator LowerBound(DataValues& values, base::Time time) {
  return values.begin() + LowerBound(span{values.data(), values.size()}, time);
}

inline DataValues::iterator UpperBound(DataValues& values, base::Time time) {
  return values.begin() + UpperBound(span{values.data(), values.size()}, time);
}
