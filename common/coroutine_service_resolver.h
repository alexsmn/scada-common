#pragma once

#include "base/any_executor.h"

#include <memory>
#include <stdexcept>

namespace scada::service_resolver {

template <typename Service>
Service& RequireSharedService(const std::shared_ptr<Service>& service) {
  if (!service)
    throw std::invalid_argument{"missing service"};
  return *service;
}

template <typename CoroutineService, typename CallbackService, typename Adapter>
CoroutineService* ResolveCoroutineService(
    const AnyExecutor& executor,
    const std::shared_ptr<CoroutineService>& coroutine_service,
    const std::shared_ptr<CallbackService>& callback_service,
    std::unique_ptr<Adapter>& adapter) {
  if (coroutine_service)
    return coroutine_service.get();

  if (!callback_service)
    return nullptr;

  if (auto* service = dynamic_cast<CoroutineService*>(callback_service.get()))
    return service;

  adapter = std::make_unique<Adapter>(executor, *callback_service);
  return adapter.get();
}

template <typename CoroutineService, typename CallbackService, typename Adapter>
CoroutineService& RequireCoroutineService(
    const AnyExecutor& executor,
    const std::shared_ptr<CoroutineService>& coroutine_service,
    const std::shared_ptr<CallbackService>& callback_service,
    std::unique_ptr<Adapter>& adapter) {
  auto* service = ResolveCoroutineService(executor, coroutine_service,
                                          callback_service, adapter);
  if (!service)
    throw std::invalid_argument{"missing service"};
  return *service;
}

}  // namespace scada::service_resolver
