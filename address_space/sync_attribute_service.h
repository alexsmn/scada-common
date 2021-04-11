#pragma once

#include "base/containers/span.h"
#include "core/attribute_service.h"

class SyncAttributeService {
 public:
  virtual ~SyncAttributeService() = default;

  virtual scada::DataValue Read(const scada::ReadValueId& read_id) = 0;

  virtual std::vector<scada::StatusCode> Write(
      base::span<const scada::WriteValueId> value_ids,
      const scada::NodeId& user_id) = 0;
};
