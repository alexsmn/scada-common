#include "timed_data/timed_data_service_factory.h"

#include "timed_data/timed_data_service_impl.h"

std::unique_ptr<TimedDataService> CreateTimedDataService(
    TimedDataContext&& context) {
  return std::make_unique<TimedDataServiceImpl>(std::move(context));
}
