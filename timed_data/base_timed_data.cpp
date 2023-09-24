#include "timed_data/base_timed_data.h"

#include "timed_data/timed_data_observer.h"

#include <sstream>

const base::Time kTimedDataCurrentOnly = base::Time::Max();

const std::vector<scada::DateTimeRange> kReadyCurrentTimeOnly = {};

BaseTimedData::BaseTimedData() {}

BaseTimedData::~BaseTimedData() {
  assert(!observers_.might_have_observers());
}

scada::DataValue BaseTimedData::GetValueAt(const base::Time& time) const {
  if (!current_.source_timestamp.is_null() && current_.source_timestamp <= time)
    return current_;

  return timed_data_view_.GetValueAt(time);
}

void BaseTimedData::AddObserver(TimedDataObserver& observer) {
  if (!observers_.HasObserver(&observer))
    observers_.AddObserver(&observer);
}

void BaseTimedData::RemoveObserver(TimedDataObserver& observer) {
  observers_.RemoveObserver(&observer);
}

void BaseTimedData::AddViewObserver(TimedDataViewObserver& observer,
                                    const scada::DateTimeRange& range) {
  timed_data_view_.AddObserver(observer, range);
  UpdateRanges();
}

void BaseTimedData::RemoveViewObserver(TimedDataViewObserver& observer) {
  timed_data_view_.RemoveObserver(observer);
  UpdateRanges();
}

void BaseTimedData::UpdateRanges() {
  if (!timed_data_view_.observed_ranges().empty())
    historical_ = true;

  // In the 'current only' mode historical values are not maintained.
  if (historical_ && !current_.is_null())
    timed_data_view_.InsertOrUpdate(current_);

  OnRangesChanged();
}

void BaseTimedData::Delete() {
  current_.qualifier.set_failed(true);
  current_.status_code = scada::StatusCode::Bad_Disconnected;

  for (auto& o : observers_)
    o.OnTimedDataDeleted();
}

void BaseTimedData::Failed() {
  PropertySet changed_mask(PROPERTY_ITEM | PROPERTY_TITLE | PROPERTY_CURRENT);
  for (auto& o : observers_)
    o.OnPropertyChanged(changed_mask);
}

bool BaseTimedData::UpdateCurrent(const scada::DataValue& value) {
  if (value == current_)
    return false;

  bool is_change =
      current_.value != value.value || current_.qualifier != value.qualifier;

  // Add new point with time of last change.
  if (historical() && is_change && !value.source_timestamp.is_null())
    timed_data_view_.InsertOrUpdate(value);

  if (is_change)
    change_time_ = value.source_timestamp;

  current_ = value;

  return true;
}

std::string BaseTimedData::DumpDebugInfo() const {
  std::stringstream stream;
  timed_data_view_.Dump(stream);
  stream << "BaseTimedData" << std::endl;
  stream << "Historical: " << historical_ << std::endl;
  return stream.str();
}

void BaseTimedData::NotifyPropertyChanged(const PropertySet& properties) {
  for (auto& o : observers_)
    o.OnPropertyChanged(properties);
}

void BaseTimedData::NotifyEventsChanged() {
  for (auto& o : observers_)
    o.OnEventsChanged();
}
