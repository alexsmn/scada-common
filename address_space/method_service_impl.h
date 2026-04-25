#pragma once

#include "scada/coroutine_services.h"
#include "scada/method_service.h"

namespace scada {
class AddressSpace;
}

struct MethodServiceImplContext {
  scada::AddressSpace& address_space_;
};

class MethodServiceImpl : private MethodServiceImplContext,
                          public scada::CoroutineMethodService {
 public:
  explicit MethodServiceImpl(const MethodServiceImplContext& context);

  void Call(const scada::NodeId& node_id,
            const scada::NodeId& method_id,
            const std::vector<scada::Variant>& arguments,
            const scada::NodeId& user_id,
            const scada::StatusCallback& callback);

  // scada::CoroutineMethodService
  Awaitable<scada::Status> Call(scada::NodeId node_id,
                                scada::NodeId method_id,
                                std::vector<scada::Variant> arguments,
                                scada::NodeId user_id) override;
};
