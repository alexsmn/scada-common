#pragma once

#include "core/data_value.h"

namespace scada {

inline bool IsCurrentDataUpdate(const DataValue& current_data, const DataValue& new_data) {
  return current_data.time.is_null() ||
         (current_data.time < new_data.time) ||
         (current_data.time == new_data.time &&
          current_data.collection_time <= new_data.collection_time);
}

} // namespace scada