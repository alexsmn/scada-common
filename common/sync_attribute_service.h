#pragma once

#include "scada/attribute_service.h"

#include <span>

class SyncAttributeService {
 public:
  virtual ~SyncAttributeService() = default;

  virtual std::vector<scada::DataValue> Read(
      const scada::ServiceContext& context,
      std::span<const scada::ReadValueId> inputs) = 0;

  virtual std::vector<scada::StatusCode> Write(
      const scada::ServiceContext& context,
      std::span<const scada::WriteValue> inputs) = 0;
};

inline scada::DataValue Read(SyncAttributeService& attribute_service,
                             const scada::ServiceContext& context,
                             const scada::ReadValueId& input) {
  std::span<const scada::ReadValueId> inputs{&input, 1};
  auto results = attribute_service.Read(context, inputs);
  assert(results.size() == 1);
  return std::move(results.front());
}
