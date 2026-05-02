#pragma once

#include "scada/attribute_service.h"

namespace scada {

class RemoteNode {
 public:
  virtual ~RemoteNode() {}

  virtual Awaitable<StatusOr<std::vector<DataValue>>> Read(
      ServiceContext context,
      std::shared_ptr<const std::vector<ReadValueId>> value_ids) = 0;
};

} // namespace scada
