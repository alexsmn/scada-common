#pragma once

#include "base/map_util.h"
#include "timed_data/timed_data_fake.h"
#include "timed_data/timed_data_service.h"

#include <unordered_map>

class FakeTimedDataService : public TimedDataService {
 public:
  std::shared_ptr<FakeTimedData> AddTimedData(const std::string& formula) {
    auto timed_data = std::make_shared<FakeTimedData>(formula);
    formulas[formula] = timed_data;
    return timed_data;
  }

  std::shared_ptr<TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::AggregateFilter& aggregation) override {
    return FindOrNull(nodes, node_id);
  }

  std::shared_ptr<TimedData> GetFormulaTimedData(
      std::string_view formula,
      const scada::AggregateFilter& aggregation) override {
    return FindOrNull(formulas, std::string{formula});
  }

  std::unordered_map<scada::NodeId, std::shared_ptr<TimedData>> nodes;
  std::unordered_map<std::string, std::shared_ptr<TimedData>> formulas;
};
