#pragma once

#include "scada/method_service.h"

namespace scada {
class AddressSpace;
}

struct MethodServiceImplContext {
  scada::AddressSpace& address_space_;
};

class MethodServiceImpl : private MethodServiceImplContext {
 public:
  explicit MethodServiceImpl(const MethodServiceImplContext& context);

  void Call(const scada::NodeId& node_id,
            const scada::NodeId& method_id,
            const std::vector<scada::Variant>& arguments,
            const scada::NodeId& user_id,
            const scada::StatusCallback& callback);
};
