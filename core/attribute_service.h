#pragma once

#include "core/attribute_ids.h"
#include "core/configuration_types.h"
#include "core/data_value.h"
#include "core/write_flags.h"

#include <cassert>
#include <functional>

namespace scada {

class NodeId;
class Status;

using StatusCallback = std::function<void(Status&&)>;
using ReadCallback =
    std::function<void(Status&&, std::vector<DataValue>&& values)>;

class AttributeService {
 public:
  virtual ~AttributeService() {}

  virtual void Read(const std::vector<ReadValueId>& value_ids,
                    const ReadCallback& callback) = 0;

  virtual void Write(const WriteValue& value,
                     const NodeId& user_id,
                     const StatusCallback& callback) = 0;
};

template <class T>
inline DataValue MakeReadResult(T&& value) {
  const auto timestamp = base::Time::Now();
  return DataValue{std::forward<T>(value), {}, timestamp, timestamp};
}

inline DataValue MakeReadError(scada::StatusCode status_code) {
  assert(scada::IsBad(status_code));
  const auto timestamp = base::Time::Now();
  return DataValue{status_code, timestamp};
}

}  // namespace scada
