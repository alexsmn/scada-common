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
                      std::vector<std::shared_ptr<TimedData>> operands);
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
  virtual void OnFromChanged() final;

  // TimedDataDelegate
  virtual void OnPropertyChanged(const PropertySet& properties) final;
  virtual void OnTimedDataCorrections(size_t count,
                                      const scada::DataValue* tvqs) final;
  virtual void OnTimedDataReady() final;
  virtual void OnTimedDataNodeModified() final;
  virtual void OnTimedDataDeleted() final;

  std::unique_ptr<ScadaExpression> expression_;
  std::vector<std::shared_ptr<TimedData>> operands_;
};

}  // namespace rt
