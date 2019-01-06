#pragma once

#include <memory>
#include <vector>

#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_delegate.h"

namespace rt {

class ScadaExpression;

class ExpressionTimedData final : public BaseTimedData,
                                  private TimedDataDelegate {
 public:
  ExpressionTimedData(std::unique_ptr<ScadaExpression> expression,
                      std::vector<std::shared_ptr<TimedData>> operands,
                      std::shared_ptr<const Logger> logger);
  ~ExpressionTimedData();

  // TimedData
  virtual std::string GetFormula(bool aliases) const override;
  virtual base::string16 GetTitle() const override;
  virtual const events::EventSet* GetEvents() const override;
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

  // TimedDataDelegate
  virtual void OnPropertyChanged(const PropertySet& properties) override;
  virtual void OnTimedDataCorrections(size_t count,
                                      const scada::DataValue* tvqs) override;
  virtual void OnTimedDataReady() override;
  virtual void OnTimedDataNodeModified() override;
  virtual void OnTimedDataDeleted() override;

  std::unique_ptr<ScadaExpression> expression_;
  std::vector<std::shared_ptr<TimedData>> operands_;

  scada::DateTime from_;
  scada::DateTime ready_from_;
};

}  // namespace rt
