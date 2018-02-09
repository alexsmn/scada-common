#pragma once

#include <memory>
#include <vector>

#include "timed_data/timed_data.h"

class TimedDataService;

namespace rt {

class ScadaExpression;

class ExpressionTimedData : public TimedData {
 public:
  ExpressionTimedData(TimedDataService& timed_data_service,
                      std::unique_ptr<ScadaExpression> expression);

  // TimedData
  virtual std::string GetFormula(bool aliases) const override;
  virtual base::string16 GetTitle() const override;
  virtual const events::EventSet* GetEvents() const override;
  virtual void Acknowledge() override;

 protected:
  // TimedData
  virtual void OnFromChanged() override;

  // TimedDataEvents
  void OnPropertyChanged(rt::TimedDataSpec& spec,
                         const rt::PropertySet& properties);
  void OnTimedDataCorrections(TimedDataSpec& spec,
                              size_t count,
                              const scada::DataValue* tvqs);
  void OnTimedDataReady(TimedDataSpec& spec);

 private:
  typedef std::vector<TimedDataSpec> OperandVector;

  // Get earliest time from which all operands are ready.
  // Returns |kTimedDataCurrentOnly| if one of operands is not ready.
  base::Time GetOperandsReadyFrom() const;

  // Fill all ranges that was requested but not calculated yet.
  // Returns false if ready range was not changed.
  bool CalculateReadyRange();

  void CalculateRange(base::Time from,
                      base::Time to,
                      std::vector<scada::DataValue>* tvqs);
  bool CalculateCurrent();

  std::unique_ptr<ScadaExpression> expression_;
  OperandVector operands_;
};

}  // namespace rt
