#pragma once

#include "core/aggregate_filter.h"

#include <memory>
#include <string_view>

class TimedData;

class TimedDataService {
 public:
  virtual ~TimedDataService() {}

  virtual std::shared_ptr<TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::AggregateFilter& aggregation) = 0;

  virtual std::shared_ptr<TimedData> GetFormulaTimedData(
      std::string_view formula,
      const scada::AggregateFilter& aggregation) = 0;
};
