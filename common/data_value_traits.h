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

  static bool UpdateValue(scada::DataValue& value,
                          const scada::DataValue& new_value) {
    assert(value.source_timestamp == new_value.source_timestamp);

    if (value.server_timestamp > new_value.server_timestamp) {
      return false;
    }

    value = new_value;
    return true;
  }
};
