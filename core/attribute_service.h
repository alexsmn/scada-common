#pragma once

#include "core/configuration_types.h"
#include "core/write_flags.h"
#include "core/attribute_ids.h"
#include "core/data_value.h"

#include <functional>

namespace scada {

class NodeId;
class Status;

using StatusCallback = std::function<void(const Status&)>;

class AttributeService {
 public:
  using ReadCallback = std::function<void(const Status&, std::vector<scada::DataValue> values)> ;
  virtual void Read(const std::vector<ReadValueId>& value_ids, const ReadCallback& callback) = 0;

  virtual void Write(const NodeId& node_id, double value, const NodeId& user_id,
                     const WriteFlags& flags, const StatusCallback& callback) = 0;
};

} // namespace scada