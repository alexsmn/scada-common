#pragma once

#include "core/attribute_service.h"

#include <gmock/gmock.h>

namespace scada {

class MockAttributeService : public AttributeService {
 public:
  MOCK_METHOD2(Read, void(const std::vector<ReadValueId>& nodes, const ReadCallback& callback));

  MOCK_METHOD5(Write, void(const NodeId& node_id, double value, const NodeId& user_id,
                           const WriteFlags& flags, const StatusCallback& callback));
};

} // namespace scada
