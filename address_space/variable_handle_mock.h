#pragma once

#include "address_space/variable_handle.h"

#include <gmock/gmock.h>

namespace scada {

class MockVariableHandle : public VariableHandle {
 public:
  MOCK_METHOD(void,
              Write,
              (AttributeId attribute_id,
               const Variant& value,
               const WriteFlags& flags,
               const NodeId& user_id,
               const StatusCallback& callback));
  MOCK_METHOD(void,
              Call,
              (const NodeId& method_id,
               const std::vector<Variant>& arguments,
               const NodeId& user_id,
               const StatusCallback& callback));
};

}  // namespace scada
