#pragma once

#include "core/attribute_ids.h"
#include "core/configuration_types.h"
#include "core/data_value.h"
#include "core/write_flags.h"

#include <functional>

namespace scada {

class NodeId;
class Status;

using StatusCallback = std::function<void(Status&&)>;
using ReadCallback = std::function<void(Status&&, std::vector<DataValue>&& values)>;

class AttributeService {
 public:
  virtual ~AttributeService() {}

  virtual void Read(const std::vector<ReadValueId>& value_ids, const ReadCallback& callback) = 0;

  virtual void Write(const NodeId& node_id, double value, const NodeId& user_id,
                     const WriteFlags& flags, const StatusCallback& callback) = 0;
};

template <class T>
inline DataValue MakeReadResult(T&& value) {
  const auto timestamp = base::Time::Now();
  return DataValue{std::forward<T>(value), {}, timestamp, timestamp};
}

} // namespace scada