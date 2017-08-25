#include "common/timed_data/timed_data_cache.h"

#include "common/timed_data/timed_data.h"

using namespace std::chrono_literals;

namespace rt {

#ifdef _DEBUG
static const auto kTimedDataCacheDuration = base::TimeDelta::FromSeconds(10);
#else
static const auto kTimedDataCacheDuration = base::TimeDelta::FromSeconds(60);
#endif

TimedDataCache::TimedDataCache(boost::asio::io_service& io_service)
    : cache_duration_{kTimedDataCacheDuration},
      timer_{io_service} {
  timer_.StartRepeating(1s, [this] { OnTimer(); });
}

TimedDataCache::~TimedDataCache() {
}

std::shared_ptr<TimedData> TimedDataCache::Find(const scada::NodeId& node_id) const {
  TimedDataMap::const_iterator i = map_.find(node_id);
  return i != map_.end() ? i->second.data : nullptr;
}

void TimedDataCache::Add(const scada::NodeId& node_id, std::shared_ptr<TimedData> data) {
  assert(map_.find(node_id) == map_.end());
  
  auto& entry = map_[node_id];
  entry.data = std::move(data);
  entry.expire_time = base::TimeTicks();
}

void TimedDataCache::OnTimer() {
  base::TimeTicks time = base::TimeTicks::Now();
  for (TimedDataMap::iterator i = map_.begin(); i != map_.end(); ) {
    auto& entry = i->second;
    if (entry.data->specs_.empty()) {
      if (entry.expire_time.is_null()) {
        // Expiration timer started.
        entry.expire_time = base::TimeTicks::Now();
      } else if (time - entry.expire_time >= cache_duration_) {
        // Expired.
        map_.erase(i++);
        continue;
      }
    }
    ++i;
  }
}

} // namespace rt
