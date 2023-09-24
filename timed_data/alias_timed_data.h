#pragma once

#include "timed_data/timed_data.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

class AliasTimedData final : public TimedData {
 public:
  explicit AliasTimedData(std::string formula);

  void SetForwarded(std::shared_ptr<TimedData> timed_data);

  // TimedData
  virtual bool IsError() const override;
  virtual const std::vector<scada::DateTimeRange>& GetReadyRanges()
      const override;
  virtual scada::DataValue GetDataValue() const override;
  virtual scada::DataValue GetValueAt(const base::Time& time) const override;
  virtual base::Time GetChangeTime() const override;
  virtual std::span<const scada::DataValue> GetValues() const override;
  virtual void AddObserver(TimedDataObserver& observer) override;
  virtual void RemoveObserver(TimedDataObserver& observer) override;
  virtual void AddViewObserver(TimedDataViewObserver& observer,
                               const scada::DateTimeRange& range) override;
  virtual void RemoveViewObserver(TimedDataViewObserver& observer) override;
  virtual std::string GetFormula(bool aliases) const override;
  virtual scada::LocalizedText GetTitle() const override;
  virtual NodeRef GetNode() const override;
  virtual bool IsAlerting() const override;
  virtual const EventSet* GetEvents() const override;
  virtual void Acknowledge() override;
  virtual std::string DumpDebugInfo() const override;

 private:
  struct DeferredData {
    DeferredData(std::string formula) : formula{std::move(formula)} {}

    const std::string formula;
    std::unordered_set<TimedDataObserver*> observers;
    std::unordered_map<TimedDataViewObserver*, scada::DateTimeRange /*range*/>
        view_observers;
  };

  bool is_forwarded() const { return data_.index() == 1; }

  DeferredData& deferred() {
    return *std::get<std::unique_ptr<DeferredData>>(data_);
  }

  const DeferredData& deferred() const {
    return const_cast<AliasTimedData*>(this)->deferred();
  }

  TimedData& forwarded() {
    return *std::get<std::shared_ptr<TimedData>>(data_);
  }

  const TimedData& forwarded() const {
    return const_cast<AliasTimedData*>(this)->forwarded();
  }

  std::variant<std::unique_ptr<DeferredData>, std::shared_ptr<TimedData>> data_;
};
