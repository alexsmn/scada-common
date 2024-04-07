#pragma once

#include "timed_data/timed_data_mock.h"
#include "timed_data/timed_data_service.h"

#include <gmock/gmock.h>

class MockTimedDataService : public TimedDataService {
 public:
  MockTimedDataService() {
    using namespace testing;

    ON_CALL(*this, GetNodeTimedData(/*node_id=*/_, /*aggregation=*/_))
        .WillByDefault(Return(default_timed_data_));

    ON_CALL(*this, GetFormulaTimedData(/*formula=*/_, /*aggregation=*/_))
        .WillByDefault(Return(default_timed_data_));
  }

  MOCK_METHOD(std::shared_ptr<TimedData>,
              GetNodeTimedData,
              (const scada::NodeId& node_id,
               const scada::AggregateFilter& aggregation));

  MOCK_METHOD(std::shared_ptr<TimedData>,
              GetFormulaTimedData,
              (std::string_view formula,
               const scada::AggregateFilter& aggregation));

  const std::shared_ptr<MockTimedData> default_timed_data_ =
      std::make_shared<testing::NiceMock<MockTimedData>>();
};
