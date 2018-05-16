#pragma once

#include "core/data_value.h"

#include <algorithm>
#include <vector>

namespace rt {

using DataValues = std::vector<scada::DataValue>;

inline DataValues::iterator LowerBound(DataValues& values, base::Time time) {
  assert(!time.is_null());
  return std::lower_bound(values.begin(), values.end(), time,
                          [](const scada::DataValue& value, base::Time time) {
                            return value.source_timestamp < time;
                          });
}

inline DataValues::iterator UpperBound(DataValues& values, base::Time time) {
  assert(!time.is_null());
  return std::upper_bound(values.begin(), values.end(), time,
                          [](base::Time time, const scada::DataValue& value) {
                            return time < value.source_timestamp;
                          });
}

inline DataValues::const_iterator LowerBound(const DataValues& values,
                                             base::Time time) {
  return LowerBound(const_cast<DataValues&>(values), time);
}

inline DataValues::const_iterator UpperBound(const DataValues& values,
                                             base::Time time) {
  return UpperBound(const_cast<DataValues&>(values), time);
}

}  // namespace rt
