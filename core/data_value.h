#pragma once

#include "core/date_time.h"
#include "core/qualifier.h"
#include "core/variant.h"

namespace scada {

class DataValue {
 public:
  DataValue() {}

  DataValue(StatusCode status_code, DateTime server_timestamp)
      : status_code{status_code}, server_timestamp{server_timestamp} {
    assert(!IsGood(status_code));
  }

  template <class T>
  DataValue(T&& value,
            Qualifier qualifier,
            DateTime source_timestamp,
            DateTime server_timestamp)
      : value(std::forward<T>(value)),
        qualifier(std::move(qualifier)),
        source_timestamp(source_timestamp),
        server_timestamp(server_timestamp) {}

  DataValue(const DataValue&) = default;
  DataValue& operator=(const DataValue&) = default;

  bool is_null() const { return value.is_null() && (qualifier.raw() == 0); }

  bool operator==(const DataValue& other) const {
    return source_timestamp == other.source_timestamp &&
           server_timestamp == other.server_timestamp && value == other.value &&
           qualifier == other.qualifier && status_code == other.status_code;
  }

  Variant value;
  Qualifier qualifier;
  DateTime source_timestamp;
  DateTime server_timestamp;
  StatusCode status_code = StatusCode::Good;
};

inline bool IsUpdate(const DataValue& current_data, const DataValue& new_data) {
  return current_data.source_timestamp.is_null() ||
         (current_data.source_timestamp < new_data.source_timestamp) ||
         (current_data.source_timestamp == new_data.source_timestamp &&
          current_data.server_timestamp <= new_data.server_timestamp);
}

}  // namespace scada
