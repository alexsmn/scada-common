#include "timed_data/timed_data_service_factory.h"

#include "timed_data/timed_data_service_impl.h"

namespace {

bool HasHistoryService(const DataServices& services) {
  return services.history_service_ || services.coroutine_history_service_;
}

void NormalizeLegacyServices(TimedDataContext& context) {
  if (HasHistoryService(context.data_services_) ||
      !context.services_.history_service) {
    return;
  }

  context.data_services_ = DataServices::FromUnownedServices(context.services_);
}

}  // namespace

std::unique_ptr<TimedDataService> CreateTimedDataService(
    TimedDataContext&& context) {
  NormalizeLegacyServices(context);
  return std::make_unique<TimedDataServiceImpl>(std::move(context));
}

std::unique_ptr<TimedDataService> CreateTimedDataService(
    CoroutineTimedDataContext&& context) {
  return std::make_unique<TimedDataServiceImpl>(std::move(context));
}
