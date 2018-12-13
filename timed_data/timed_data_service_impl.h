#pragma once

#include "base/memory/weak_ptr.h"
#include "base/timed_cache.h"
#include "timed_data/timed_data_context.h"
#include "timed_data/timed_data_service.h"

class AliasTimedData;
class Logger;
template <class Key, class Value>
class TimedDataCache;

namespace rt {
class TimedDataImpl;
}

class TimedDataServiceImpl final : private TimedDataContext,
                                   public TimedDataService {
 public:
  TimedDataServiceImpl(TimedDataContext&& context,
                       std::shared_ptr<const Logger> logger);
  virtual ~TimedDataServiceImpl();

  virtual std::shared_ptr<rt::TimedData> GetFormulaTimedData(
      base::StringPiece formula,
      const scada::Aggregation& aggregation) override;
  virtual std::shared_ptr<rt::TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::Aggregation& aggregation) override;

 private:
  std::shared_ptr<rt::TimedData> GetAliasTimedData(
      base::StringPiece alias,
      const scada::Aggregation& aggregation);

  const std::shared_ptr<const Logger> logger_;

  TimedCache<std::pair<scada::NodeId, scada::Aggregation>,
             std::shared_ptr<rt::TimedDataImpl>>
      node_id_cache_;
  TimedCache<std::pair<std::string, scada::Aggregation>,
             std::shared_ptr<AliasTimedData>>
      alias_cache_;

  const std::shared_ptr<rt::TimedData> null_timed_data_;

  base::WeakPtrFactory<TimedDataServiceImpl> weak_ptr_factory_{this};
};
