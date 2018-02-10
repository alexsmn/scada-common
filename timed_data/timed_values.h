#pragma once

#include <map>

#include "core/data_value.h"

namespace rt {

struct TimedDataEntry {
  VQ vq;
  base::Time collection_time;
};

typedef std::map<base::Time, TimedDataEntry> TimedValues;
typedef TimedValues::const_iterator TimedValuesIterator;

} // namespace rt
