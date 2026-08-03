#include "timed_data/expression_timed_data.h"

#include "base/check.h"
#include "common/scada_expression.h"
#include "common/timed_data_util.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_spec.h"
#include "timed_data/timed_data_util.h"

ExpressionTimedData::ExpressionTimedData(
    std::unique_ptr<ScadaExpression> expression,
    std::vector<std::shared_ptr<TimedData>> operands)
    : expression_{std::move(expression)}, operands_{std::move(operands)} {
  for (const auto& operand : operands_) {
    operand->AddObserver(*this);
    operand->AddViewObserver(*this, {from_, kTimedDataCurrentOnly});
  }
  CalculateCurrent();
}

ExpressionTimedData::~ExpressionTimedData() {
  for (const auto& operand : operands_) {
    operand->RemoveObserver(*this);
    operand->RemoveViewObserver(*this);
  }
}

std::string ExpressionTimedData::GetFormula(bool aliases) const {
  return expression_->Format(aliases);
}

scada::LocalizedText ExpressionTimedData::GetTitle() const {
  return scada::ToLocalizedText(expression_->Format(true));
}

void ExpressionTimedData::OnObservedRangesChanged() {
  size_t num_operands = operands_.size();

  // connect operands
  for (size_t i = 0; i < num_operands; ++i) {
    operands_[i]->AddObserver(*this);
    operands_[i]->AddViewObserver(*this, {from_, kTimedDataCurrentOnly});
  }

  if (historical()) {
    UpdateReadyRange();
  }

  CalculateCurrent();
}

void ExpressionTimedData::Acknowledge() {
  for (size_t i = 0; i < operands_.size(); ++i)
    operands_[i]->Acknowledge();
}

scada::Time ExpressionTimedData::GetOperandsReadyFrom() const {
  scada::Time ready_from = scada::kNullTime;

  scada::TimeRange range{from_, kTimedDataCurrentOnly};
  for (size_t i = 0; i < operands_.size(); ++i) {
    const auto& operand = *operands_[i];
    scada::Time operand_ready_from =
        GetReadyFrom(operand.GetReadyRanges(), range);

    if (operand_ready_from == kTimedDataCurrentOnly)
      return kTimedDataCurrentOnly;

    scada::base::Check(!scada::IsNull(operand_ready_from));
    if (operand_ready_from > ready_from)
      ready_from = operand_ready_from;
  }

  return scada::IsNull(ready_from) ? kTimedDataCurrentOnly : ready_from;
}

void ExpressionTimedData::CalculateValuesInRange(
    const scada::TimeRange& range) {
  scada::base::Check(!scada::IsNull(range.first));
  scada::base::Check(scada::IsNull(range.second) || range.first <= range.second);

  // Coalesce the per-value inserts below into a single observer notification.
  auto batch = buffer_.BeginUpdate();

  std::vector<size_t> iters(operands_.size());

  // Initialize calculation iterators and initial values.
  for (size_t i = 0; i < operands_.size(); ++i) {
    const auto& operand = *operands_[i];
    const auto& values = operand.GetValues();

    size_t& iterator = iters[i];
    iterator = LowerBound(values, range.first);

    auto& initial_value = expression_->items[i].value;
    if (iterator != values.size()) {
      initial_value = values[iterator];
    } else {
      scada::base::Check(values.back().source_timestamp <= range.first);
      initial_value = values.back();
    }
  }

  // Run calculation.
  for (;;) {
    // Setup operand values and find next calculation time.
    scada::Time update_time = scada::kNullTime;
    scada::Qualifier total_qualifier;
    bool calculation_finished = true;

    for (size_t i = 0; i < operands_.size(); ++i) {
      const auto& operand = *operands_[i];
      const auto& values = operand.GetValues();

      ScadaExpression::Item& item = expression_->items[i];

      if (scada::IsNull(update_time) || update_time < item.value.source_timestamp)
        update_time = item.value.source_timestamp;

      // Check iterator reached end.
      size_t& iterator = iters[i];
      if (iterator == values.size()) {
        continue;
      }
      const scada::Time& time = values[iterator].source_timestamp;

      // Warning: condition "time >= to" is incorrect here.
      if (!scada::IsNull(range.second) && time > range.second) {
        continue;
      }

      calculation_finished = false;

      // Setup operand value.
      item.value = values[iterator];

      // Update total qualifier.
      if (item.value.qualifier.bad())
        total_qualifier.set_bad(true);

      ++iterator;
    }

    // Stop if all iterator are at the end.
    if (calculation_finished)
      break;

    scada::base::Check(!scada::IsNull(update_time));

    // calculate
    auto total_value = expression_->Calculate();
    if (!total_value.is_null()) {
      // A recomputed historical sample has no server timestamp: kNullTime,
      // not a default-constructed scada::Time (the Unix epoch).
      scada::DataValue tvq(std::move(total_value), total_qualifier, update_time,
                           scada::kNullTime);

      // The insert may be rejected in favor of an existing value with the
      // same timestamp; that is data-dependent, not an invariant.
      buffer_.InsertOrUpdate(tvq);
    }
  }
}

void ExpressionTimedData::UpdateReadyRange() {
  scada::base::Check(historical());

  scada::Time operands_ready_from = GetOperandsReadyFrom();
  if (operands_ready_from == kTimedDataCurrentOnly) {
    return;
  }

  scada::base::Check(!scada::IsNull(operands_ready_from));

  auto range = scada::TimeRange{operands_ready_from, ready_from_};
  CalculateValuesInRange(range);
  buffer_.AddReadyRange(range);
}

bool ExpressionTimedData::CalculateCurrent() {
  scada::Qualifier total_qualifier;

  size_t num_operands = operands_.size();

  scada::Time max_update_time = scada::kNullTime;
  for (size_t i = 0; i < num_operands; ++i) {
    const auto& operand = *operands_[i];

    expression_->items[i].value = scada::DataValue();

    const auto& cur = operand.GetDataValue();
    if (scada::IsNull(cur.source_timestamp))
      continue;

    // calculate value
    expression_->items[i].value = cur;

    // find maximal change and update times
    const scada::Time& update_time = cur.source_timestamp;
    if (!scada::IsNull(update_time)) {
      if (scada::IsNull(max_update_time) || update_time > max_update_time)
        max_update_time = update_time;
    }

    // update flags
    if (cur.qualifier.bad())
      total_qualifier.set_bad(true);
  }

  scada::Variant total_value;
  if (auto calculated = expression_->CalculateStatus(); calculated.ok()) {
    total_value = std::move(*calculated);
  }

  auto now = scada::Now();
  if (num_operands == 0)
    max_update_time = now;

  scada::DataValue data_value{std::move(total_value), total_qualifier,
                              max_update_time, now};
  return UpdateCurrent(data_value);
}

void ExpressionTimedData::OnTimedDataUpdates(
    std::span<const scada::DataValue> values) {
  scada::base::Check(historical());
  scada::base::Check(!values.empty());
  scada::base::Check(values.front().source_timestamp >= ready_from_);

  scada::TimeRange range{values.front().source_timestamp,
                             values.back().source_timestamp};

  // Clear then recompute the affected range as one logical change; the batch
  // inside CalculateValuesInRange coalesces the recomputed values into a single
  // notification, so no manual NotifyUpdates is needed.
  buffer_.ClearRange(range);
  CalculateValuesInRange(range);
}

void ExpressionTimedData::OnPropertyChanged(const PropertySet& properties) {
  if (properties.is_current_changed() && CalculateCurrent()) {
    NotifyPropertyChanged(PropertySet(PROPERTY_CURRENT));
  }
}

void ExpressionTimedData::OnTimedDataReady() {
  scada::base::Check(historical());

  UpdateReadyRange();
}

const EventSet* ExpressionTimedData::GetEvents() const {
  return nullptr;
}
