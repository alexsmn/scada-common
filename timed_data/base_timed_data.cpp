#include "timed_data/base_timed_data.h"

#include "base/format.h"
#include "common/event_manager.h"
#include "timed_data/timed_data_spec.h"

namespace rt {

const base::Time kTimedDataCurrentOnly = base::Time::Max();

BaseTimedData::BaseTimedData() {}

BaseTimedData::~BaseTimedData() {
  assert(!observers_.might_have_observers());
}

scada::DataValue BaseTimedData::GetValueAt(const base::Time& time) const {
  if (!historical())
    return current_.source_timestamp <= time ? current_ : scada::DataValue();

  auto i = rt::LowerBound(values_, time);

  if (i != values_.end() && i->source_timestamp == time)
    return *i;

  if (i == values_.begin())
    return {};

  --i;
  if (i == values_.end())
    return {};

  return *i;
}

void BaseTimedData::AddObserver(TimedDataDelegate& observer) {
  observers_.AddObserver(&observer);
}

void BaseTimedData::RemoveObserver(TimedDataDelegate& observer) {
  observers_.RemoveObserver(&observer);
}

void BaseTimedData::SetFrom(base::Time from) {
  if (from >= from_)
    return;

  // When data switches from current-only to historical, add current value
  // into historical values.
  if (!historical() && !current_.source_timestamp.is_null())
    UpdateHistory(current_);

  from_ = from;

  OnFromChanged();
}

void BaseTimedData::UpdateReadyFrom(base::Time time) {
  if (time >= ready_from_)
    return;

  ready_from_ = time;

  if (time < from_)
    from_ = time;
}

void BaseTimedData::ResetReadyFrom() {
  ready_from_ = kTimedDataCurrentOnly;
}

void BaseTimedData::Write(double value,
                          const scada::NodeId& user_id,
                          const scada::WriteFlags& flags,
                          const StatusCallback& callback) const {
  if (callback)
    callback(scada::StatusCode::Bad_Disconnected);
}

void BaseTimedData::Call(const scada::NodeId& method_id,
                         const std::vector<scada::Variant>& arguments,
                         const scada::NodeId& user_id,
                         const StatusCallback& callback) const {
  if (callback)
    callback(scada::StatusCode::Bad_Disconnected);
}

void BaseTimedData::Delete() {
  current_.qualifier.set_failed(true);

  for (auto& o : observers_)
    o.OnTimedDataDeleted();
}

void BaseTimedData::Failed() {
  PropertySet changed_mask(PROPERTY_ITEM | PROPERTY_TITLE | PROPERTY_CURRENT);
  for (auto& o : observers_)
    o.OnPropertyChanged(changed_mask);
}

bool CheckTvqsSorted(size_t count, const scada::DataValue* tvqs) {
  if (count <= 1)
    return true;

  for (size_t i = 0; i < count - 1; ++i) {
    if (tvqs[i].source_timestamp >= tvqs[i + 1].source_timestamp)
      return false;
  }

  return true;
}

void BaseTimedData::NotifyTimedDataCorrection(size_t count,
                                              const scada::DataValue* tvqs) {
  assert(count > 0);
  assert(CheckTvqsSorted(count, tvqs));

  for (auto& o : observers_)
    o.OnTimedDataCorrections(count, tvqs);
}

void BaseTimedData::NotifyPropertyChanged(const PropertySet& properties) {
  for (auto& o : observers_)
    o.OnPropertyChanged(properties);
}

void BaseTimedData::NotifyDataReady() {
  for (auto& o : observers_)
    o.OnTimedDataReady();
}

void BaseTimedData::NotifyEventsChanged() {
  for (auto& o : observers_)
    o.OnEventsChanged();
}

bool BaseTimedData::UpdateHistory(const scada::DataValue& value) {
  if (value.source_timestamp.is_null())
    return false;

  // An optimization for the back inserts.
  if (values_.empty() ||
      values_.back().source_timestamp < value.source_timestamp) {
    values_.emplace_back(value);
    return true;
  }

  auto i = LowerBound(values_, value.source_timestamp);
  if (i != values_.end() && i->source_timestamp == value.source_timestamp) {
    if (i->server_timestamp > value.server_timestamp)
      return false;

    *i = value;
    return true;

  } else {
    values_.insert(i, value);
    return true;
  }
}

bool BaseTimedData::UpdateCurrent(const scada::DataValue& value) {
  if (value == current_)
    return false;

  bool is_change =
      current_.value != value.value || current_.qualifier != value.qualifier;

  // Add new point with time of last change.
  if (historical() && is_change && !value.source_timestamp.is_null())
    UpdateHistory(value);

  if (is_change)
    change_time_ = value.source_timestamp;

  current_ = value;
  return true;
}

void BaseTimedData::ClearRange(base::Time from, base::Time to) {
  assert(!from.is_null());
  assert(to.is_null() || from <= to);

  auto i = LowerBound(values_, from);
  auto j = to.is_null() ? values_.end() : UpperBound(values_, to);
  values_.erase(i, j);
}

}  // namespace rt
