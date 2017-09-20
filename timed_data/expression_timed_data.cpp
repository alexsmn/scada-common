#include "expression_timed_data.h"

#include "base/strings/sys_string_conversions.h"
#include "timed_data/scada_expression.h"
#include "timed_data/timed_data_spec.h"

namespace rt {

ExpressionTimedData::ExpressionTimedData(
    TimedDataService& timed_data_service,
    std::unique_ptr<ScadaExpression> expression)
    : expression_(std::move(expression)) {
  size_t num_operands = expression_->items.size();

  operands_.resize(num_operands);
  for (size_t i = 0; i < num_operands; ++i) {
    ScadaExpression::Item& item = expression_->items[i];
    TimedDataSpec& oper = operands_[i];
    oper.set_delegate(this);
    oper.Connect(timed_data_service, item.name);
  }

  CalculateCurrent();
}

std::string ExpressionTimedData::GetFormula(bool aliases) const {
  return expression_->Format(aliases);
}

base::string16 ExpressionTimedData::GetTitle() const {
  return base::SysNativeMBToWide(expression_->Format(true));
}

void ExpressionTimedData::OnFromChanged() {
  size_t num_operands = operands_.size();

  // connect operands
  for (size_t i = 0; i < num_operands; ++i)
    operands_[i].SetFrom(from());

  if (historical())
    CalculateReadyRange();

  CalculateCurrent();
}

void ExpressionTimedData::Acknowledge() {
  for (size_t i = 0; i < operands_.size(); ++i)
    operands_[i].Acknowledge();
}

base::Time ExpressionTimedData::GetOperandsReadyFrom() const {
  base::Time ready_from;
  
  for (size_t i = 0; i < operands_.size(); ++i) {
    const TimedDataSpec& operand = operands_[i];
    base::Time operand_ready_from = operand.ready_from();

    if (operand_ready_from == kTimedDataCurrentOnly)
      return kTimedDataCurrentOnly;

    assert(!operand_ready_from.is_null());
    if (operand_ready_from > ready_from)
      ready_from = operand_ready_from;
  }
  
  return ready_from.is_null() ? kTimedDataCurrentOnly : ready_from;
}

void ExpressionTimedData::CalculateRange(base::Time from, base::Time to,
                                         std::vector<scada::DataValue>* tvqs) {
  assert(!from.is_null());
  assert(to.is_null() || from <= to);
                                           
  std::vector<TimedVQMap::const_iterator> iters(operands_.size());

  // Initialize calculation iterators and initial values.
  for (size_t i = 0; i < operands_.size(); ++i) {
    TimedDataSpec& operand = operands_[i];
    assert(operand.ready_from() <= from);

    const TimedVQMap* values = operand.values();
    assert(values);

    TimedVQMap::const_iterator& iterator = iters[i];
    iterator = values->lower_bound(from);
    
    auto& initial_value = expression_->items[i].value;
    if (iterator != values->end()) {
      initial_value = scada::DataValue(iterator->second.vq.value,
                                       iterator->second.vq.qualifier,
                                       iterator->first,
                                       iterator->second.collection_time);
    } else {
      TimedVQMap::const_iterator last = --values->end();
      assert(last->first <= from);
      initial_value = scada::DataValue(last->second.vq.value,
                                       last->second.vq.qualifier,
                                       last->first,
                                       last->second.collection_time);
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
      TimedDataSpec& operand = operands_[i];
      const TimedVQMap* values = operand.values();
      assert(values);

      ScadaExpression::Item& item = expression_->items[i];
      
      if (update_time.is_null() || update_time < item.value.source_timestamp)
        update_time = item.value.source_timestamp;

      // Check iterator reached end.
      TimedVQMap::const_iterator& iterator = iters[i];
      if (iterator == values->end())
        continue;
      const base::Time& time = iterator->first;
      
      // Warning: condition "time >= to" is incorrect here.
      if (!to.is_null() && time > to)
        continue;
        
      calculation_finished = false;

      // Setup operand value.
      item.value = scada::DataValue(iterator->second.vq.value,
                                    iterator->second.vq.qualifier,
                                    iterator->first,
                                    iterator->second.collection_time);

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
      scada::DataValue tvq(std::move(total_value), total_qualifier, update_time, base::Time());

      bool updated = UpdateMap(tvq);
      assert(updated);
      
      if (tvqs) {
        // Check values are ordered.
        assert(tvqs->empty() || (*tvqs)[tvqs->size() - 1].source_timestamp <= tvq.source_timestamp);
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
  assert(operands_ready_from <= ready_from());

  CalculateRange(operands_ready_from, ready_from(), NULL);
  UpdateReadyFrom(operands_ready_from);

  return true;
}

bool ExpressionTimedData::CalculateCurrent() {
  scada::Qualifier total_qualifier;

  size_t num_operands = operands_.size();

  base::Time max_update_time;
  for (size_t i = 0; i < num_operands; ++i) {
    TimedDataSpec& oper = operands_[i];

    expression_->items[i].value = scada::DataValue();

    const auto& cur = oper.current();
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
  
  if (!num_operands)
    max_update_time = base::Time::Now();
  
  scada::DataValue tvq(std::move(total_value), total_qualifier, max_update_time, base::Time());
  return UpdateCurrent(tvq);
}

void ExpressionTimedData::OnTimedDataCorrections(TimedDataSpec& spec, size_t count, const scada::DataValue* tvqs) {
  assert(historical());
  assert(count > 0);
  assert(tvqs[0].source_timestamp >= ready_from());

  ClearRange(tvqs[0].source_timestamp, tvqs[count - 1].source_timestamp);

  std::vector<scada::DataValue> changed_tvqs;
  CalculateRange(tvqs[0].source_timestamp, tvqs[count - 1].source_timestamp, &changed_tvqs);

  if (!changed_tvqs.empty())
    NotifyTimedDataCorrection(changed_tvqs.size(), &changed_tvqs[0]);
}

void ExpressionTimedData::OnPropertyChanged(rt::TimedDataSpec& spec,
                                            const rt::PropertySet& properties) {
  if (properties.is_current_changed()) {
    if (CalculateCurrent())
      NotifyPropertyChanged(PropertySet(PROPERTY_CURRENT));
  }
}

void ExpressionTimedData::OnTimedDataReady(TimedDataSpec& spec) {
  assert(historical());
  if (CalculateReadyRange())
    NotifyDataReady();
}

void ExpressionTimedData::OnTimedDataNodeModified(
    rt::TimedDataSpec& spec, const scada::PropertyIds& property_ids) {
}

void ExpressionTimedData::OnTimedDataDeleted(TimedDataSpec& spec) {
}

const events::EventSet* ExpressionTimedData::GetEvents() const {
  return NULL;
}

} // namespace rt
