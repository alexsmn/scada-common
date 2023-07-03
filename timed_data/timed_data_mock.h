#pragma once

#include "timed_data/timed_data.h"

#include <gmock/gmock.h>
#include <vector>

class MockTimedData : public TimedData {
 public:
  MockTimedData() {
    using namespace testing;

    ON_CALL(*this, AddObserver(_, _))
        .WillByDefault(Invoke([this](TimedDataDelegate& observer,
                                     const scada::DateTimeRange& range) {
          observers_.emplace_back(&observer);
        }));

    ON_CALL(*this, RemoveObserver(_))
        .WillByDefault(Invoke([this](TimedDataDelegate& observer) {
          std::erase(observers_, &observer);
        }));
  }

  MOCK_METHOD(bool, IsError, (), (const override));

  MOCK_METHOD(const std::vector<scada::DateTimeRange>&,
              GetReadyRanges,
              (),
              (const override));

  MOCK_METHOD(scada::DataValue, GetDataValue, (), (const override));

  MOCK_METHOD(base::Time, GetChangeTime, (), (const override));

  MOCK_METHOD(const DataValues*, GetValues, (), (const override));

  MOCK_METHOD(scada::DataValue,
              GetValueAt,
              (const base::Time& time),
              (const override));

  MOCK_METHOD(void,
              AddObserver,
              (TimedDataDelegate & observer, const scada::DateTimeRange& range),
              (override));

  MOCK_METHOD(void, RemoveObserver, (TimedDataDelegate & observer), (override));

  MOCK_METHOD(std::string, GetFormula, (bool aliases), (const override));

  MOCK_METHOD(scada::LocalizedText, GetTitle, (), (const override));

  MOCK_METHOD(NodeRef, GetNode, (), (const override));

  MOCK_METHOD(bool, IsAlerting, (), (const override));

  MOCK_METHOD(const EventSet*, GetEvents, (), (const override));

  MOCK_METHOD(void, Acknowledge, (), (override));

  MOCK_METHOD(void,
              Write,
              (double value,
               const scada::NodeId& user_id,
               const scada::WriteFlags& flags,
               const StatusCallback& callback),
              (const override));

  MOCK_METHOD(void,
              Call,
              (const scada::NodeId& method_id,
               const std::vector<scada::Variant>& arguments,
               const scada::NodeId& user_id,
               const StatusCallback& callback),
              (const override));

  MOCK_METHOD(std::string, DumpDebugInfo, (), (const override));

  TimedDataDelegate* last_observer() {
    return observers_.empty() ? nullptr : observers_.back();
  }

  std::vector<TimedDataDelegate*> observers_;
};
