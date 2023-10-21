#pragma once

#include "timed_data/timed_data.h"

class ErrorTimedData final : public TimedData {
 public:
  ErrorTimedData(std::string formula, scada::LocalizedText title)
      : formula_{std::move(formula)}, title_{std::move(title)} {}

  virtual bool IsError() const override { return true; }
  virtual const std::vector<scada::DateTimeRange>& GetReadyRanges()
      const override;
  virtual scada::DataValue GetDataValue() const override { return {}; }
  virtual const scada::DataValue* GetValueAt(
      const base::Time& time) const override {
    return nullptr;
  }
  virtual base::Time GetChangeTime() const override { return {}; }
  virtual std::span<const scada::DataValue> GetValues() const override {
    return {};
  }
  virtual void AddObserver(TimedDataObserver& observer) override {}
  virtual void RemoveObserver(TimedDataObserver& observer) override {}
  virtual void AddViewObserver(TimedDataViewObserver& observer,
                               const scada::DateTimeRange& range) override {}
  virtual void RemoveViewObserver(TimedDataViewObserver& observer) override {}
  virtual std::string GetFormula(bool aliases) const override {
    return formula_;
  }
  virtual scada::LocalizedText GetTitle() const override;
  virtual NodeRef GetNode() const override { return NodeRef{}; }
  virtual bool IsAlerting() const override { return false; }
  virtual const EventSet* GetEvents() const override { return nullptr; }
  virtual void Acknowledge() override {}
  virtual std::string DumpDebugInfo() const override;

 private:
  const std::string formula_;
  const scada::LocalizedText title_;

  inline static const std::vector<scada::DateTimeRange> kReadyRanges{
      {scada::DateTime::Min(), scada::DateTime::Max()}};
};

inline scada::LocalizedText ErrorTimedData::GetTitle() const {
  return title_;
}

inline const std::vector<scada::DateTimeRange>& ErrorTimedData::GetReadyRanges()
    const {
  return kReadyRanges;
}

std::string ErrorTimedData::DumpDebugInfo() const {
  return "ErrorTimedData";
}
