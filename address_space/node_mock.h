#pragma once

#include "address_space/node.h"

#include <gmock/gmock.h>

namespace scada {

class MockNode : public Node {
 public:
  MOCK_METHOD(NodeClass, GetNodeClass, (), (const override));

  MOCK_METHOD(void, Startup, (), (override));
  MOCK_METHOD(void, Shutdown, (), (override));
};

}  // namespace scada
