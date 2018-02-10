#include "timed_data/base_timed_data.h"

#include "base/format.h"
#include "common/event_manager.h"
#include "timed_data/timed_data_spec.h"

namespace rt {

const base::Time kTimedDataCurrentOnly =
    base::Time::FromInternalValue(std::numeric_limits<int64>::max());

BaseTimedData::BaseTimedData()
    : alerting_(false),
      from_(kTimedDataCurrentOnly),
      ready_from_(kTimedDataCurrentOnly) {}

BaseTimedData::~BaseTimedData() {
  assert(!observers_.might_have_observers());
}

scada::DataValue BaseTimedData::GetValueAt(const base::Time& time) const {
  if (!historical())
    return current_.source_timestamp <= time ? current_ : scada::DataValue();

  TimedVQMap::const_iterator i = map_.lower_bound(time);
  if (i == map_.end())
    return scada::DataValue();

  --i;
  if (i == map_.end())
    return scada::DataValue();

  return scada::DataValue(i->second.vq.value, i->second.vq.qualifier, i->first,
                          i->second.server_timestamp);
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
    UpdateMap(current_);

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
                          const scada::WriteFlags& flags,
                          const StatusCallback& callback) const {
  if (callback)
    callback(scada::StatusCode::Bad_Disconnected);
}

void BaseTimedData::Call(const scada::NodeId& method_id,
                         const std::vector<scada::Variant>& arguments,
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

bool BaseTimedData::UpdateMap(const scada::DataValue& value) {
  if (value.source_timestamp.is_null())
    return false;

  TimedDataEntry& entry = map_[value.source_timestamp];
  if (entry.server_timestamp > value.server_timestamp)
    return false;

  entry.vq.value = value.value;
  entry.vq.qualifier = value.qualifier;
  entry.server_timestamp = value.server_timestamp;
  return true;
}

bool BaseTimedData::UpdateCurrent(const scada::DataValue& value) {
  if (value == current_)
    return false;

  bool is_change =
      current_.value != value.value || current_.qualifier != value.qualifier;

  // Add new point with time of last change.
  if (historical() && is_change && !value.source_timestamp.is_null())
    UpdateMap(value);

  if (is_change)
    change_time_ = value.source_timestamp;

  current_ = value;
  return true;
}

void BaseTimedData::ClearRange(base::Time from, base::Time to) {
  assert(!from.is_null());
  assert(to.is_null() || from <= to);

  TimedVQMap::iterator i = map_.lower_bound(from);
  if (i == map_.end())
    return;

  assert(i->first >= from);

  TimedVQMap::iterator j = to.is_null() ? map_.end() : map_.lower_bound(to);

  map_.erase(i, j);
}

}  // namespace rt
