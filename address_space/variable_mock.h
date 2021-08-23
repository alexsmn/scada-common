#pragma once

#include "address_space/variable.h"

#include <gmock/gmock.h>

namespace scada {

class MockVariable : public Variable {
 public:
  MOCK_METHOD(DataValue, GetValue, (), (const override));
  MOCK_METHOD(Status, SetValue, (const DataValue& data_value), (override));
  MOCK_METHOD(DateTime, GetChangeTime, (), (const override));
};

}  // namespace scada
