#pragma once

#include "timed_data/timed_data_context.h"
#include "timed_data/timed_data_service.h"

namespace rt {
class TimedDataCache;
}

class TimedDataServiceImpl : public TimedDataService,
                             private TimedDataContext {
 public:
  explicit TimedDataServiceImpl(const TimedDataContext& context);
  virtual ~TimedDataServiceImpl();

  virtual std::shared_ptr<rt::TimedData> GetNodeTimedData(const scada::NodeId& node_id) override;

 private:
  std::unique_ptr<rt::TimedDataCache> timed_data_cache_;
};