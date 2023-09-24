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
  virtual scada::DataValue GetValueAt(const base::Time& time) const override {
    return {};
  }
  virtual base::Time GetChangeTime() const override { return {}; }
  virtual const std::vector<scada::DataValue>* GetValues() const override {
    return nullptr;
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
  virtual void Write(double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const StatusCallback& callback) const override;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback) const override;
  virtual std::string DumpDebugInfo() const override;

 private:
  const std::string formula_;
  const scada::LocalizedText title_;
};

inline scada::LocalizedText ErrorTimedData::GetTitle() const {
  return title_;
}

inline void ErrorTimedData::Write(double value,
                                  const scada::NodeId& user_id,
                                  const scada::WriteFlags& flags,
                                  const StatusCallback& callback) const {
  callback(scada::StatusCode::Bad_Disconnected);
}
inline void ErrorTimedData::Call(const scada::NodeId& method_id,
                                 const std::vector<scada::Variant>& arguments,
                                 const scada::NodeId& user_id,
                                 const StatusCallback& callback) const {
  callback(scada::StatusCode::Bad_Disconnected);
}

inline const std::vector<scada::DateTimeRange>& ErrorTimedData::GetReadyRanges()
    const {
  static const std::vector<scada::DateTimeRange> kReadyRanges{
      {scada::DateTime::Min(), scada::DateTime::Max()}};
  return kReadyRanges;
}

std::string ErrorTimedData::DumpDebugInfo() const {
  return "ErrorTimedData";
}
