#pragma once

#include <map>

#include "core/tvq.h"

namespace rt {

struct TimedDataEntry {
  scada::VQ vq;
  base::Time collection_time;
};

typedef std::map<base::Time, TimedDataEntry> TimedVQMap;

} // namespace rt