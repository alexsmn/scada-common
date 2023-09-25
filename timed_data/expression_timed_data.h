#pragma once

#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_observer.h"

#include <memory>
#include <vector>

class ScadaExpression;

class ExpressionTimedData final : public BaseTimedData,
                                  private TimedDataObserver,
                                  private TimedDataViewObserver {
 public:
  ExpressionTimedData(std::unique_ptr<ScadaExpression> expression,
                      std::vector<std::shared_ptr<TimedData>> operands);
  ~ExpressionTimedData();

  // TimedData
  virtual std::string GetFormula(bool aliases) const override;
  virtual scada::LocalizedText GetTitle() const override;
  virtual const EventSet* GetEvents() const override;
  virtual void Acknowledge() override;

 private:
  // Get earliest time from which all operands are ready.
  // Returns |kTimedDataCurrentOnly| if one of operands is not ready.
  base::Time GetOperandsReadyFrom() const;

  // Fill all ranges that was requested but not calculated yet.
  // Returns false if ready range was not changed.
  bool CalculateReadyRange();

  void CalculateRange(const scada::DateTimeRange& range,
                      std::vector<scada::DataValue>* tvqs);
  bool CalculateCurrent();

  // TimedData
  virtual void OnRangesChanged() override;

  // TimedDataObserver
  virtual void OnPropertyChanged(const PropertySet& properties) override;

  // TimedDataViewObserver
  virtual void OnTimedDataUpdates(
      std::span<const scada::DataValue> values) override;
  virtual void OnTimedDataReady() override;

  std::unique_ptr<ScadaExpression> expression_;
  std::vector<std::shared_ptr<TimedData>> operands_;

  scada::DateTime from_ = kTimedDataCurrentOnly;
  scada::DateTime ready_from_ = kTimedDataCurrentOnly;
};
