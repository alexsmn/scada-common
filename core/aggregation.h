#pragma once

#include "core/date_time.h"
#include "core/node_id.h"

namespace scada {

struct Aggregation {
  Duration interval;
  NodeId aggregation_id;
};

}  // namespace scada
