#include "timed_data/expression_timed_data.h"

#include "common/scada_expression.h"
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

void ExpressionTimedData::OnRangesChanged() {
  size_t num_operands = operands_.size();

  // connect operands
  for (size_t i = 0; i < num_operands; ++i) {
    operands_[i]->AddObserver(*this);
    operands_[i]->AddViewObserver(*this, {from_, kTimedDataCurrentOnly});
  }

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

  scada::DateTimeRange range{from_, kTimedDataCurrentOnly};
  for (size_t i = 0; i < operands_.size(); ++i) {
    const auto& operand = *operands_[i];
    base::Time operand_ready_from =
        GetReadyFrom(operand.GetReadyRanges(), range);

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
      assert(values.back().source_timestamp <= range.first);
      initial_value = values.back();
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
      const auto& operand = *operands_[i];
      const auto& values = operand.GetValues();

      ScadaExpression::Item& item = expression_->items[i];

      if (update_time.is_null() || update_time < item.value.source_timestamp)
        update_time = item.value.source_timestamp;

      // Check iterator reached end.
      size_t& iterator = iters[i];
      if (iterator == values.size()) {
        continue;
      }
      const base::Time& time = values[iterator].source_timestamp;

      // Warning: condition "time >= to" is incorrect here.
      if (!range.second.is_null() && time > range.second) {
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

    assert(!update_time.is_null());

    // calculate
    auto total_value = expression_->Calculate();
    if (!total_value.is_null()) {
      scada::DataValue tvq(std::move(total_value), total_qualifier, update_time,
                           base::Time());

      bool updated = timed_data_view_.InsertOrUpdate(tvq);
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

  auto range = scada::DateTimeRange{operands_ready_from, ready_from_};
  CalculateRange(range, nullptr);
  timed_data_view_.AddReadyRange(range);

  return true;
}

bool ExpressionTimedData::CalculateCurrent() {
  scada::Qualifier total_qualifier;

  size_t num_operands = operands_.size();

  base::Time max_update_time;
  for (size_t i = 0; i < num_operands; ++i) {
    const auto& operand = *operands_[i];

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

  auto now = base::Time::Now();
  if (num_operands == 0)
    max_update_time = now;

  scada::DataValue data_value{std::move(total_value), total_qualifier,
                              max_update_time, now};
  return UpdateCurrent(data_value);
}

void ExpressionTimedData::OnTimedDataUpdates(
    std::span<const scada::DataValue> values) {
  assert(historical());
  assert(values.empty());
  assert(values.front().source_timestamp >= ready_from_);

  scada::DateTimeRange range{values.front().source_timestamp,
                             values.back().source_timestamp};

  timed_data_view_.ClearRange(range);

  std::vector<scada::DataValue> changed_values;
  CalculateRange(range, &changed_values);

  if (!changed_values.empty()) {
    timed_data_view_.NotifyUpdates(changed_values);
  }
}

void ExpressionTimedData::OnPropertyChanged(const PropertySet& properties) {
  if (properties.is_current_changed() && CalculateCurrent()) {
    NotifyPropertyChanged(PropertySet(PROPERTY_CURRENT));
  }
}

void ExpressionTimedData::OnTimedDataReady() {
  assert(historical());

  if (CalculateReadyRange()) {
    timed_data_view_.NotifyReady();
  }
}

const EventSet* ExpressionTimedData::GetEvents() const {
  return nullptr;
}
