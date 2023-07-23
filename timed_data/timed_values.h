#pragma once

#include <map>

#include "scada/data_value.h"

struct TimedDataEntry {
  VQ vq;
  base::Time collection_time;
};

typedef std::map<base::Time, TimedDataEntry> TimedValues;
typedef TimedValues::const_iterator TimedValuesIterator;
