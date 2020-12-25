#pragma once

#include "timed_data/timed_data_service.h"

#include <gmock/gmock.h>

class MockTimedDataService : public TimedDataService {
 public:
  MOCK_METHOD(std::shared_ptr<TimedData>,
              GetNodeTimedData,
              (const scada::NodeId& node_id,
               const scada::AggregateFilter& aggregation));

  MOCK_METHOD(std::shared_ptr<TimedData>,
              GetFormulaTimedData,
              (std::string_view formula,
               const scada::AggregateFilter& aggregation));
};
