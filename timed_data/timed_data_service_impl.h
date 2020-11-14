#pragma once

#include "base/memory/weak_ptr.h"
#include "base/timed_cache.h"
#include "timed_data/timed_data_context.h"
#include "timed_data/timed_data_service.h"

class TimedDataImpl;
class AliasTimedData;
class Logger;

class TimedDataServiceImpl final : private TimedDataContext,
                                   public TimedDataService {
 public:
  TimedDataServiceImpl(TimedDataContext&& context,
                       std::shared_ptr<const Logger> logger);
  virtual ~TimedDataServiceImpl();

  virtual std::shared_ptr<TimedData> GetFormulaTimedData(
      std::string_view formula,
      const scada::AggregateFilter& aggregation) override;
  virtual std::shared_ptr<TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::AggregateFilter& aggregation) override;

 private:
  std::shared_ptr<TimedData> GetAliasTimedData(
      std::string_view alias,
      const scada::AggregateFilter& aggregation);

  const std::shared_ptr<const Logger> logger_;

  TimedCache<std::pair<scada::NodeId, scada::AggregateFilter>,
             std::shared_ptr<TimedDataImpl>>
      node_id_cache_;
  TimedCache<std::pair<std::string, scada::AggregateFilter>,
             std::shared_ptr<AliasTimedData>>
      alias_cache_;

  const std::shared_ptr<TimedData> null_timed_data_;

  base::WeakPtrFactory<TimedDataServiceImpl> weak_ptr_factory_{this};
};
