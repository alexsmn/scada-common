#pragma once

#include "core/date_time.h"
#include "core/node_id.h"

namespace scada {

struct Aggregation {
  bool is_null() const { return interval.is_zero(); }

  bool operator<(const Aggregation& other) const {
    return std::tie(interval, aggregation_id) <
           std::tie(other.interval, other.aggregation_id);
  }

  Duration interval;
  NodeId aggregation_id;
};

}  // namespace scada
