#include "timed_data/expression_timed_data.h"

#include "timed_data/scada_expression.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"

namespace rt {

ExpressionTimedData::ExpressionTimedData(
    std::unique_ptr<ScadaExpression> expression,
    std::vector<std::shared_ptr<TimedData>> operands)
    : expression_{std::move(expression)}, operands_{std::move(operands)} {
  for (auto& operand : operands_)
    operand->AddObserver(*this, {from_, rt::kTimedDataCurrentOnly});
  CalculateCurrent();
}

ExpressionTimedData::~ExpressionTimedData() {
  for (auto& operand : operands_)
    operand->RemoveObserver(*this);
}

std::string ExpressionTimedData::GetFormula(bool aliases) const {
  return expression_->Format(aliases);
}

scada::LocalizedText ExpressionTimedData::GetTitle() const {
  return scada::ToLocalizedText(expression_->Format(true));
}

void ExpressionTimedData::OnFromChanged() {
  size_t num_operands = operands_.size();

  // connect operands
  for (size_t i = 0; i < num_operands; ++i)
    operands_[i]->AddObserver(*this, {from_, rt::kTimedDataCurrentOnly});

  if (historical())
    CalculateReadyRange();

  CalculateCurrent();
}

void ExpressionTimedData::Acknowledge() {
  for (size_t i = 0; i < operands_.size(); ++i)
    operands_[i]->Acknowledge();
}

base::Time ExpressionTimedData::GetOperandsReadyFrom() const {
  base::Time ready_from;

  for (size_t i = 0; i < operands_.size(); ++i) {
    const auto& operand = *operands_[i];
    base::Time operand_ready_from = operand.GetReadyFrom();

    if (operand_ready_from == kTimedDataCurrentOnly)
      return kTimedDataCurrentOnly;

    assert(!operand_ready_from.is_null());
    if (operand_ready_from > ready_from)
      ready_from = operand_ready_from;
  }

  return ready_from.is_null() ? kTimedDataCurrentOnly : ready_from;
}

void ExpressionTimedData::CalculateRange(const scada::DateTimeRange& range,
                                         std::vector<scada::DataValue>* tvqs) {
  assert(!range.first.is_null());
  assert(range.second.is_null() || range.first <= range.second);

  std::vector<DataValues::const_iterator> iters(operands_.size());

  // Initialize calculation iterators and initial values.
  for (size_t i = 0; i < operands_.size(); ++i) {
    auto& operand = *operands_[i];
    assert(operand.GetReadyFrom() <= range.first);

    const DataValues* values = operand.GetValues();
    assert(values);

    DataValues::const_iterator& iterator = iters[i];
    iterator = LowerBound(*values, range.first);

    auto& initial_value = expression_->items[i].value;
    if (iterator != values->end()) {
      initial_value = *iterator;
    } else {
      assert(values->back().source_timestamp <= range.first);
      initial_value = values->back();
    }
  }

  if (tvqs)
    tvqs->reserve(64);

  // Run calculation.
  for (;;) {
    // Setup operand values and find next calculation time.
    base::Time update_time;
    scada::Qualifier total_qualifier;
    bool calculation_finished = true;

    for (size_t i = 0; i < operands_.size(); ++i) {
      auto& operand = *operands_[i];
      const DataValues* values = operand.GetValues();
      assert(values);

      ScadaExpression::Item& item = expression_->items[i];

      if (update_time.is_null() || update_time < item.value.source_timestamp)
        update_time = item.value.source_timestamp;

      // Check iterator reached end.
      DataValues::const_iterator& iterator = iters[i];
      if (iterator == values->end())
        continue;
      const base::Time& time = iterator->source_timestamp;

      // Warning: condition "time >= to" is incorrect here.
      if (!range.second.is_null() && time > range.second)
        continue;

      calculation_finished = false;

      // Setup operand value.
      item.value = *iterator;

      // Update total qualifier.
      if (item.value.qualifier.bad())
        total_qualifier.set_bad(true);

      ++iterator;
    }

    // Stop if all iterator are at the end.
    if (calculation_finished)
      break;

    assert(!update_time.is_null());

    // calculate
    auto total_value = expression_->Calculate();
    if (!total_value.is_null()) {
      scada::DataValue tvq(std::move(total_value), total_qualifier, update_time,
                           base::Time());

      bool updated = UpdateHistory(tvq);
      assert(updated);

      if (tvqs) {
        // Check values are ordered.
        assert(tvqs->empty() || (*tvqs)[tvqs->size() - 1].source_timestamp <=
                                    tvq.source_timestamp);
        tvqs->push_back(tvq);
      }
    }
  }
}

bool ExpressionTimedData::CalculateReadyRange() {
  assert(historical());

  base::Time operands_ready_from = GetOperandsReadyFrom();
  if (operands_ready_from == kTimedDataCurrentOnly)
    return false;

  assert(!operands_ready_from.is_null());
  // Operands can't be less ready than we already know.
  assert(operands_ready_from <= ready_from_);

  auto range = scada::DateTimeRange{operands_ready_from, ready_from_};
  CalculateRange(range, NULL);
  SetReady(range);

  return true;
}

bool ExpressionTimedData::CalculateCurrent() {
  scada::Qualifier total_qualifier;

  size_t num_operands = operands_.size();

  base::Time max_update_time;
  for (size_t i = 0; i < num_operands; ++i) {
    auto& operand = *operands_[i];

    expression_->items[i].value = scada::DataValue();

    const auto& cur = operand.GetDataValue();
    if (cur.source_timestamp.is_null())
      continue;

    // calculate value
    expression_->items[i].value = cur;

    // find maximal change and update times
    const base::Time& update_time = cur.source_timestamp;
    if (!update_time.is_null()) {
      if (max_update_time.is_null() || update_time > max_update_time)
        max_update_time = update_time;
    }

    // update flags
    if (cur.qualifier.bad())
      total_qualifier.set_bad(true);
  }

  scada::Variant total_value;
  try {
    total_value = expression_->Calculate();
  } catch (const std::exception&) {
  }

  if (num_operands == 0)
    max_update_time = base::Time::Now();

  scada::DataValue tvq(std::move(total_value), total_qualifier, max_update_time,
                       base::Time());
  return UpdateCurrent(tvq);
}

void ExpressionTimedData::OnTimedDataCorrections(size_t count,
                                                 const scada::DataValue* tvqs) {
  assert(historical());
  assert(count > 0);
  assert(tvqs[0].source_timestamp >= ready_from_);

  scada::DateTimeRange range{tvqs[0].source_timestamp,
                             tvqs[count - 1].source_timestamp};

  Clear(range);

  std::vector<scada::DataValue> changed_tvqs;
  CalculateRange(range, &changed_tvqs);

  if (!changed_tvqs.empty())
    NotifyTimedDataCorrection(changed_tvqs.size(), &changed_tvqs[0]);
}

void ExpressionTimedData::OnPropertyChanged(const PropertySet& properties) {
  if (properties.is_current_changed()) {
    if (CalculateCurrent())
      NotifyPropertyChanged(PropertySet(PROPERTY_CURRENT));
  }
}

void ExpressionTimedData::OnTimedDataReady() {
  assert(historical());
  if (CalculateReadyRange())
    NotifyDataReady();
}

void ExpressionTimedData::OnTimedDataNodeModified() {}

void ExpressionTimedData::OnTimedDataDeleted() {}

const events::EventSet* ExpressionTimedData::GetEvents() const {
  return NULL;
}

}  // namespace rt
