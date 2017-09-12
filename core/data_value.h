#pragma once

#include "base/time/time.h"
#include "core/qualifier.h"
#include "core/variant.h"

namespace scada {

class DataValue {
 public:
  DataValue() {}

  DataValue(StatusCode status_code) : status_code{status_code} {}

  template<class T>
  DataValue(T&& value, Qualifier qualifier, base::Time time, base::Time collection_time)
      : value(std::forward<T>(value)),
        qualifier(std::move(qualifier)),
        time(time),
        collection_time(collection_time) {
  }

  bool is_null() const {
    return value.is_null() && (qualifier.raw() == 0);
  }

  bool operator==(const DataValue& other) const {
    return time == other.time &&
           collection_time == other.collection_time &&
           value == other.value &&
           qualifier == other.qualifier;
  }

  Variant value;
  Qualifier qualifier;
  base::Time time;
  base::Time collection_time;
  StatusCode status_code = StatusCode::Good;
};

} // namespace scada