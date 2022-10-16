#pragma once

#include "common/variable_handle.h"

#include <gmock/gmock.h>

namespace scada {

class MockVariableHandle : public VariableHandle {
 public:
  MOCK_METHOD(void,
              Write,
              (const std::shared_ptr<const scada::ServiceContext>& context,
               const scada::WriteValue& input,
               const scada::StatusCallback& callback),
              (override));
  MOCK_METHOD(void,
              Call,
              (const NodeId& method_id,
               const std::vector<Variant>& arguments,
               const NodeId& user_id,
               const StatusCallback& callback),
              (override));
};

}  // namespace scada
