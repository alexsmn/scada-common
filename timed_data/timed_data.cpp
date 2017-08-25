#include "timed_data.h"

#include "base/format.h"
#include "core/monitored_item_service.h"
#include "timed_data/timed_data_spec.h"
#include "common/event_manager.h"

namespace rt {

const char kConnectingName[] = "#намнб!";

const base::Time kTimedDataCurrentOnly =
    base::Time::FromInternalValue(std::numeric_limits<int64>::max());

TimedData::TimedData()
    : alerting_(false),
      from_(kTimedDataCurrentOnly),
      ready_from_(kTimedDataCurrentOnly) {
}

TimedData::~TimedData() {
  assert(specs_.empty());
}

scada::DataValue TimedData::GetValueAt(const base::Time& time) const {
  if (!historical())
    return current_.time <= time ? current_ : scada::DataValue();

  TimedVQMap::const_iterator i = map_.lower_bound(time);
  if (i == map_.end())
    return scada::DataValue();

  --i;
  if (i == map_.end())
    return scada::DataValue();

  return scada::DataValue(i->second.vq.value,
                          i->second.vq.qualifier,
                          i->first,
                          i->second.collection_time);
}

void TimedData::Subscribe(TimedDataSpec& spec) {
  assert(specs_.find(&spec) == specs_.end());
  specs_.insert(&spec);
}

void TimedData::Unsubscribe(TimedDataSpec& spec) {
  assert(specs_.find(&spec) != specs_.end());
  specs_.erase(&spec);
}

void TimedData::SetFrom(base::Time time) {
  if (time >= from_)
    return;
  
  // When data switches from current-only to historical, add current value
  // into historical values.
  if (!historical() && !current_.time.is_null())
    UpdateMap(current_);

  from_ = time;

  OnFromChanged();
}

void TimedData::UpdateReadyFrom(base::Time time) {
  if (time >= ready_from_)
    return;

  ready_from_ = time;

  if (time < from_)
    from_ = time;
}

void TimedData::ResetReadyFrom() {
  ready_from_ = kTimedDataCurrentOnly;
}

void TimedData::Write(double value, const scada::WriteFlags& flags,
                      const StatusCallback& callback) const {
  if (callback)
    callback(scada::StatusCode::Bad_WrongMethodId);
}

void TimedData::Call(const scada::NodeId& method_id,
                     const std::vector<scada::Variant>& arguments,
                     const StatusCallback& callback) const {
  if (callback)
    callback(scada::StatusCode::Bad_WrongMethodId);
}

void TimedData::Delete() {
  current_.qualifier.set_failed(true);

  // notify specs
  while (!specs_.empty()) {
    TimedDataSpec& spec = **specs_.begin();
    spec.Reset();
    if (spec.delegate())
      spec.delegate()->OnTimedDataDeleted(spec);
  }
}

void TimedData::Failed() {
  // notify specs
  PropertySet changed_mask(PROPERTY_ITEM |
                                     PROPERTY_TITLE |
                                     PROPERTY_CURRENT);
  while (!specs_.empty()) {
    TimedDataSpec& spec = **specs_.begin();
    spec.Reset();
    if (spec.delegate())
      spec.delegate()->OnPropertyChanged(spec, changed_mask);
  }
}

bool CheckTvqsSorted(size_t count, const scada::DataValue* tvqs) {
  if (count <= 1)
    return true;
  for (size_t i = 0; i < count - 1; ++i)
    if (tvqs[i].time >= tvqs[i + 1].time)
      return false;
  return true;
}

void TimedData::NotifyTimedDataCorrection(size_t count, const scada::DataValue* tvqs) {
  assert(count > 0);
  assert(CheckTvqsSorted(count, tvqs));
  
  for (TimedDataSpecSet::iterator i = specs_.begin(); i != specs_.end(); ) {
    TimedDataSpec& spec = **i++;
    if (spec.delegate() && spec.historical()) {
      // For each spec find range it's subscribed on.
      size_t spec_count = count;
      const scada::DataValue* spec_tvqs = tvqs;
      while (spec_count && spec_tvqs[0].time < spec.from()) {
        ++spec_tvqs;
        --spec_count;
      }
      if (spec_count)
        spec.delegate()->OnTimedDataCorrections(spec, spec_count, spec_tvqs);
    }
  }
}

void TimedData::NotifyPropertyChanged(const PropertySet& properties) {
  for (TimedDataSpecSet::iterator i = specs_.begin(); i != specs_.end(); ) {
    TimedDataSpec& spec = **i++;
    if (spec.delegate())
      spec.delegate()->OnPropertyChanged(spec, properties);
  }
}

void TimedData::NotifyDataReady() {
  for (TimedDataSpecSet::iterator i = specs_.begin(); i != specs_.end(); ) {
    TimedDataSpec& spec = **i++;
    if (spec.delegate() && spec.historical())
      spec.delegate()->OnTimedDataReady(spec);
  }
}

void TimedData::NotifyEventsChanged(const events::EventSet& events) {
  assert(alerting_ == !events.empty());

  for (TimedDataSpecSet::iterator i = specs_.begin(); i != specs_.end(); ) {
    TimedDataSpec& spec = **i++;
    if (spec.delegate())
      spec.delegate()->OnEventsChanged(spec, events);
  }
}

bool TimedData::UpdateMap(const scada::DataValue& value) {
  if (value.time.is_null())
    return false;

  TimedDataEntry& entry = map_[value.time];
  if (entry.collection_time > value.collection_time)
    return false;

  entry.vq.value = value.value;
  entry.vq.qualifier = value.qualifier;
  entry.collection_time = value.collection_time;
  return true;
}

bool TimedData::UpdateCurrent(const scada::DataValue& value) {
  if (value == current_)
    return false;

  bool is_change = current_.value != value.value ||
                   current_.qualifier != value.qualifier;

  // Add new point with time of last change.
  if (historical() && is_change && !value.time.is_null())
    UpdateMap(value);

  if (is_change)
    change_time_ = value.time;

  current_ = value;
  return true;
}

void TimedData::ClearRange(base::Time from, base::Time to) {
	assert(!from.is_null());
	assert(to.is_null() || from <= to);

	TimedVQMap::iterator i = map_.lower_bound(from);
	if (i == map_.end())
		return;

	assert(i->first >= from);

	TimedVQMap::iterator j = to.is_null() ? map_.end() : map_.lower_bound(to);

	map_.erase(i, j);
}

} // namespace rt
