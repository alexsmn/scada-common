#pragma once

#include "scada/coroutine_services.h"
#include "scada/method_service.h"

namespace scada {
class AddressSpace;
}  // namespace scada

class AddressSpaceMethodService : public scada::MethodService {
 public:
  explicit AddressSpaceMethodService(scada::AddressSpace& address_space)
      : address_space_{address_space} {}

  // scada::MethodService
  virtual Awaitable<scada::Status> Call(
      scada::NodeId node_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments,
      scada::ServiceContext context) override;

 private:
  [[maybe_unused]] scada::AddressSpace& address_space_;
};

inline Awaitable<scada::Status> AddressSpaceMethodService::Call(
    scada::NodeId /*node_id*/,
    scada::NodeId /*method_id*/,
    std::vector<scada::Variant> /*arguments*/,
    scada::ServiceContext /*context*/) {
  co_return scada::Status{scada::StatusCode::Bad_WrongMethodId};
}
