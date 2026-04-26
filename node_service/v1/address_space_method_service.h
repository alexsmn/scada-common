#pragma once

#include "scada/coroutine_services.h"
#include "scada/method_service.h"

namespace scada {
class AddressSpace;
}  // namespace scada

class AddressSpaceMethodService : public scada::MethodService,
                                  public scada::CoroutineMethodService {
 public:
  explicit AddressSpaceMethodService(scada::AddressSpace& address_space)
      : address_space_{address_space} {}

  // scada::MethodService
  virtual void Call(const scada::NodeId& node_id,
                    const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) override;

  // scada::CoroutineMethodService
  virtual Awaitable<scada::Status> Call(
      scada::NodeId node_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments,
      scada::NodeId user_id) override;

 private:
  [[maybe_unused]] scada::AddressSpace& address_space_;
};

inline void AddressSpaceMethodService::Call(
    const scada::NodeId& /*node_id*/,
    const scada::NodeId& /*method_id*/,
    const std::vector<scada::Variant>& /*arguments*/,
    const scada::NodeId& /*user_id*/,
    const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad_WrongMethodId);
}

inline Awaitable<scada::Status> AddressSpaceMethodService::Call(
    scada::NodeId /*node_id*/,
    scada::NodeId /*method_id*/,
    std::vector<scada::Variant> /*arguments*/,
    scada::NodeId /*user_id*/) {
  co_return scada::Status{scada::StatusCode::Bad_WrongMethodId};
}
