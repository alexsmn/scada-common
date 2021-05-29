#pragma once

#include "common/type_system.h"

#include <gmock/gmock.h>

class MockTypeSystem : public TypeSystem {
 public:
  MOCK_METHOD(bool,
              IsSubtypeOf,
              (const scada::NodeId& type_definition_id,
               const scada::NodeId& supertype_id),
              (const override));
};
