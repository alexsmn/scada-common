#pragma once

#include "timed_data/timed_data.h"
#include "timed_data/timed_data_observer.h"
#include "timed_data/timed_data_property.h"

#include <gmock/gmock.h>
#include <vector>

class MockTimedData : public TimedData {
 public:
  MockTimedData() {
    using namespace testing;

    ON_CALL(*this, GetDataValue()).WillByDefault(Invoke([&] {
      return data_value_;
    }));

    ON_CALL(*this, AddObserver(_))
        .WillByDefault(Invoke([this](TimedDataObserver& observer) {
          observers_.emplace_back(&observer);
        }));

    ON_CALL(*this, RemoveObserver(_))
        .WillByDefault(Invoke([this](TimedDataObserver& observer) {
          std::erase(observers_, &observer);
        }));

    ON_CALL(*this, AddViewObserver(_, _))
        .WillByDefault(Invoke([this](TimedDataViewObserver& observer,
                                     const scada::DateTimeRange& range) {
          view_observers_.emplace_back(&observer);
        }));

    ON_CALL(*this, RemoveViewObserver(_))
        .WillByDefault(Invoke([this](TimedDataViewObserver& observer) {
          std::erase(view_observers_, &observer);
        }));
  }

  MOCK_METHOD(bool, IsError, (), (const override));

  MOCK_METHOD(const std::vector<scada::DateTimeRange>&,
              GetReadyRanges,
              (),
              (const override));

  MOCK_METHOD(scada::DataValue, GetDataValue, (), (const override));

  MOCK_METHOD(scada::DateTime, GetChangeTime, (), (const override));

  MOCK_METHOD(std::span<const scada::DataValue>,
              GetValues,
              (),
              (const override));

  MOCK_METHOD(const scada::DataValue*,
              GetValueAt,
              (const scada::DateTime& time),
              (const override));

  MOCK_METHOD(void, AddObserver, (TimedDataObserver & observer), (override));

  MOCK_METHOD(void, RemoveObserver, (TimedDataObserver & observer), (override));

  MOCK_METHOD(void,
              AddViewObserver,
              (TimedDataViewObserver & observer,
               const scada::DateTimeRange& range),
              (override));

  MOCK_METHOD(void,
              RemoveViewObserver,
              (TimedDataViewObserver & observer),
              (override));

  MOCK_METHOD(std::string, GetFormula, (bool aliases), (const override));

  MOCK_METHOD(scada::LocalizedText, GetTitle, (), (const override));

  MOCK_METHOD(NodeRef, GetNode, (), (const override));

  MOCK_METHOD(bool, IsAlerting, (), (const override));

  MOCK_METHOD(const EventSet*, GetEvents, (), (const override));

  MOCK_METHOD(void, Acknowledge, (), (override));

  MOCK_METHOD(std::string, DumpDebugInfo, (), (const override));

  TimedDataObserver* last_observer() {
    return observers_.empty() ? nullptr : observers_.back();
  }

  TimedDataViewObserver* last_view_observer() {
    return view_observers_.empty() ? nullptr : view_observers_.back();
  }

  void UpdateData(scada::DataValue&& data_value) {
    data_value_ = std::move(data_value);

    for (TimedDataObserver* obs : observers_) {
      obs->OnPropertyChanged(
          PropertySet{TimedDataPropertyID::PROPERTY_CURRENT});
    }
  }

  scada::DataValue data_value_;

  std::vector<TimedDataObserver*> observers_;
  std::vector<TimedDataViewObserver*> view_observers_;
};
