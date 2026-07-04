#pragma once

#include "scada/method_service.h"

namespace scada {
class AddressSpace;
}

struct MethodServiceImplContext {
  scada::AddressSpace& address_space_;
};

class MethodServiceImpl : private MethodServiceImplContext,
                          public scada::MethodService {
 public:
  explicit MethodServiceImpl(const MethodServiceImplContext& context);

  // scada::MethodService
  Awaitable<scada::Status> Call(scada::NodeId node_id,
                                scada::NodeId method_id,
                                std::vector<scada::Variant> arguments,
                                scada::ServiceContext context) override;
};
