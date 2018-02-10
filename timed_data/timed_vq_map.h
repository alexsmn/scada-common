#pragma once

#include <map>

#include "core/date_time.h"
#include "core/tvq.h"

namespace rt {

struct TimedDataEntry {
  scada::VQ vq;
  scada::DateTime server_timestamp;
};

typedef std::map<scada::DateTime /*source_timestamp*/, TimedDataEntry> TimedVQMap;

} // namespace rt