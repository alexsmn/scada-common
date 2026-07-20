#include "timed_data/base_timed_data.h"

#include "timed_data/timed_data_observer.h"
#include "timed_data/timed_data_property.h"

#include <sstream>

#if defined(SCADA_USE_CORE_MODULE)
// Modules-pilot consumer (SCADA_CXX_MODULES=ON): base/scada names come from
// the scada.core facade. The import sits after the textual includes because
// the reverse order trips an AppleClang 21 declaration-merging bug in libc++.
import scada.core;
#else
#include "base/check.h"
#endif

#include "base/debug_util.h"

const scada::DateTime kTimedDataCurrentOnly = scada::DateTime::Max();

const std::vector<scada::DateTimeRange> kReadyCurrentTimeOnly = {};

BaseTimedData::BaseTimedData() {
  buffer_.set_observed_ranges_updated_handler(
      std::bind_front(&BaseTimedData::UpdateObservedRanges, this));
}

BaseTimedData::~BaseTimedData() {
  scada::base::Check(!observers_.might_have_observers());
}

const scada::DataValue* BaseTimedData::GetValueAt(
    const scada::DateTime& time) const {
  if (!current_.source_timestamp.is_null() &&
      current_.source_timestamp <= time) {
    return &current_;
  }

  return buffer_.GetValueAt(time);
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
  buffer_.AddObserver(observer, range);
}

void BaseTimedData::RemoveViewObserver(TimedDataViewObserver& observer) {
  buffer_.RemoveObserver(observer);
}

void BaseTimedData::UpdateObservedRanges(bool has_observers) {
  if (has_observers) {
    historical_ = true;
  }

  // In the 'current only' mode historical values are not maintained.
  if (historical_ && !current_.is_null()) {
    buffer_.InsertOrUpdate(current_);
  }

  OnObservedRangesChanged();
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
    buffer_.InsertOrUpdate(value);

  if (is_change)
    change_time_ = value.source_timestamp;

  current_ = value;

  return true;
}

std::string BaseTimedData::DumpDebugInfo() const {
  std::stringstream stream;
  buffer_.Dump(stream);
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
