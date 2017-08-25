#pragma once

#include "base/time/time.h"
#include "base/timer.h"
#include "core/node_id.h"

#include <map>
#include <memory>

namespace rt {

class TimedData;

class TimedDataCache {
 public:
  explicit TimedDataCache(boost::asio::io_service& io_service);
  virtual ~TimedDataCache();

  void Add(const scada::NodeId& node_id, std::shared_ptr<TimedData> data);

  std::shared_ptr<TimedData> Find(const scada::NodeId& node_id) const;

 private:
  struct CacheEntry {
    std::shared_ptr<TimedData> data;
    base::TimeTicks expire_time;
  };

  typedef std::map<scada::NodeId, CacheEntry> TimedDataMap;

  void OnTimer();
  
  base::TimeDelta cache_duration_;
  TimedDataMap map_;
  
  Timer timer_;
};

} // namespace rt
