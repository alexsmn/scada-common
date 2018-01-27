#pragma once

#include "base/timer/timer.h"

#include <map>
#include <memory>

template <class Value>
bool IsTimedCacheExpired(const Value& value);

template <class Key, class Value>
class TimedCache {
 public:
  TimedCache();

  template<class T>
  void Add(const Key& key, T&& value);

  Value Find(const Key& key) const;

 private:
  struct CacheEntry {
    template<class T>
    explicit CacheEntry(T&& value) : value{std::forward<T>(value)} {}

    Value value;
    base::TimeTicks expiration_time;
  };

  void OnTimer();

  const base::TimeDelta cache_duration_ =
      base::TimeDelta::FromSeconds(kCacheDurationS);

  std::map<Key, CacheEntry> map_;

  base::RepeatingTimer timer_;

#ifdef _DEBUG
  static const unsigned kCacheDurationS = 10;
#else
  static const unsigned kCacheDurationS = 60;
#endif
};

template <class Key, class Value>
inline TimedCache<Key, Value>::TimedCache() {
  timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(1), this,
               &TimedCache::OnTimer);
}

template <class Key, class Value>
inline Value TimedCache<Key, Value>::Find(
    const Key& key) const {
  auto i = map_.find(key);
  return i != map_.end() ? i->second.value : nullptr;
}

template <class Key, class Value>
template <class T>
inline void TimedCache<Key, Value>::Add(const Key& key,
                                            T&& value) {
  assert(map_.find(key) == map_.end());
  map_.emplace(std::piecewise_construct,
      std::forward_as_tuple(key),
      std::forward_as_tuple(std::forward<T>(value)));
}

template <class Key, class Value>
inline void TimedCache<Key, Value>::OnTimer() {
  base::TimeTicks time = base::TimeTicks::Now();
  for (auto i = map_.begin(); i != map_.end();) {
    auto& entry = i->second;
    if (IsTimedCacheExpired(entry.value)) {
      if (entry.expiration_time.is_null()) {
        // Expiration timer started.
        entry.expiration_time = base::TimeTicks::Now();
      } else if (time - entry.expiration_time >= cache_duration_) {
        // Expired.
        map_.erase(i++);
        continue;
      }
    }
    ++i;
  }
}