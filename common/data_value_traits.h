#pragma once

#include "scada/data_value.h"

template <typename T>
struct TimedDataTraits;

template <>
struct TimedDataTraits<scada::DataValue> {
  static constexpr scada::DateTime timestamp(
      const scada::DataValue& data_value) {
    return data_value.source_timestamp;
  }

  static bool IsLesser(const scada::DataValue& a, const scada::DataValue& b) {
    return std::tie(a.source_timestamp, a.server_timestamp) <
           std::tie(b.source_timestamp, b.server_timestamp);
  }
};
