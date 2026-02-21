#pragma once

#include "common/timed_data_util.h"
#include "common/data_value_traits.h"
#include "scada/data_value.h"
#include "timed_data/timed_data.h"

#include "base/utf_convert.h"

class FakeTimedData : public TimedData {
 public:
  FakeTimedData() = default;
  explicit FakeTimedData(const std::string& formula)
      : formula{formula},
        title{UtfConvert<char16_t>(formula)} {}

  const std::vector<scada::DateTimeRange>& GetReadyRanges() const override {
    return ready_ranges;
  }

  scada::DataValue GetDataValue() const override { return {}; }

  scada::DateTime GetChangeTime() const override { return {}; }

  std::span<const scada::DataValue> GetValues() const override {
    return data_values;
  }

  const scada::DataValue* GetValueAt(
      const scada::DateTime& time) const override {
    return ::GetValueAt(std::span{data_values}, time);
  }

  void AddObserver(TimedDataObserver& observer) override {}

  void RemoveObserver(TimedDataObserver& observer) override {}

  void AddViewObserver(TimedDataViewObserver& observer,
                       const scada::DateTimeRange& range) override {}

  void RemoveViewObserver(TimedDataViewObserver& observer) override {}

  std::string GetFormula(bool aliases) const override { return formula; }

  scada::LocalizedText GetTitle() const override { return title; }

  NodeRef GetNode() const override { return {}; }

  bool IsAlerting() const override { return false; }

  const EventSet* GetEvents() const override { return nullptr; }

  void Acknowledge() override {}

  std::string DumpDebugInfo() const override { return "FakeTimedData"; }

  std::vector<scada::DataValue> data_values;
  std::string formula;
  scada::LocalizedText title;

  std::vector<scada::DateTimeRange> ready_ranges;
};
