#pragma once

#include "core/data_value.h"

namespace scada {

inline bool IsCurrentDataUpdate(const DataValue& current_data, const DataValue& new_data) {
  return current_data.source_timestamp.is_null() ||
         (current_data.source_timestamp < new_data.source_timestamp) ||
         (current_data.source_timestamp == new_data.source_timestamp &&
          current_data.server_timestamp <= new_data.server_timestamp);
}

} // namespace scada