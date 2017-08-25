#include "common/timed_data/timed_data_service_impl.h"

#include "common/timed_data/timed_data_cache.h"
#include "common/timed_data/timed_data_impl.h"

TimedDataServiceImpl::TimedDataServiceImpl(const TimedDataContext& context)
    : TimedDataContext(context),
      timed_data_cache_{std::make_unique<rt::TimedDataCache>(io_service_)} {
}

TimedDataServiceImpl::~TimedDataServiceImpl() {
}

std::shared_ptr<rt::TimedData> TimedDataServiceImpl::GetNodeTimedData(const scada::NodeId& node_id) {
  auto cached_data = timed_data_cache_->Find(node_id);
  if (cached_data)
    return cached_data;

  TimedDataContext& context = *this;
  auto timed_data = std::make_shared<rt::TimedDataImpl>(node_id, context, nullptr);

  timed_data_cache_->Add(node_id, timed_data);
  
  return timed_data;
}
