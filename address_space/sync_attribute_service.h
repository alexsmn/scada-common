#pragma once

#include "base/containers/span.h"
#include "core/attribute_service.h"

class SyncAttributeService {
 public:
  virtual ~SyncAttributeService() = default;

  virtual std::vector<scada::DataValue> Read(
      const scada::ServiceContext& context,
      base::span<const scada::ReadValueId> inputs) = 0;

  virtual std::vector<scada::StatusCode> Write(
      const scada::ServiceContext& context,
      base::span<const scada::WriteValue> inputs) = 0;
};
