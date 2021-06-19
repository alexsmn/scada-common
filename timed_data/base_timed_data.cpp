#include "timed_data/base_timed_data.h"

#include "base/format.h"
#include "base/format_time.h"
#include "base/strings/stringprintf.h"
#include "common/data_value_util.h"
#include "common/event_fetcher.h"
#include "common/interval_util.h"
#include "base/debug_util.h"
#include "timed_data/timed_data_spec.h"
#include "timed_data/timed_data_util.h"

#include <sstream>

#include "base/debug_util-inl.h"

namespace {

// Should be even.
const size_t kDumpMaxValueCount = 10;

template <class T>
void Dump(std::ostream& stream, const std::vector<T>& v) {
  for (auto& e : v)
    stream << e << std::endl;
}

void Dump(std::ostream& stream, scada::DateTime time) {
  if (time.is_min())
    stream << "min";
  else if (time.is_max())
    stream << "max";
  else if (time.is_null())
    stream << "null";
  else
    stream << time;
}

void Dump(std::ostream& stream, const scada::DateTimeRange& range) {
  stream << "(";
  Dump(stream, range.first);
  stream << ",";
  Dump(stream, range.second);
  stream << ")";
}

void Dump(
    std::ostream& stream,
    const std::map<TimedDataDelegate*, scada::DateTimeRange>& observer_ranges) {
  for (auto& p : observer_ranges) {
    Dump(stream, p.second);
    stream << std::endl;
  }
}

template <class It>
void DumpRange(std::ostream& stream, It first, It last) {
  for (auto i = first; i != last; ++i)
    stream << *i << std::endl;
}

}  // namespace

const base::Time kTimedDataCurrentOnly = base::Time::Max();

const std::vector<scada::DateTimeRange> kReadyCurrentTimeOnly = {};

BaseTimedData::BaseTimedData(std::shared_ptr<const Logger> logger)
    : logger_{std::move(logger)} {}

BaseTimedData::~BaseTimedData() {
  assert(!observers_.might_have_observers());
}

scada::DataValue BaseTimedData::GetValueAt(const base::Time& time) const {
  if (!current_.source_timestamp.is_null() && current_.source_timestamp <= time)
    return current_;

  auto i = LowerBound(values_, time);

  if (i != values_.end() && i->source_timestamp == time)
    return *i;

  if (i == values_.begin())
    return {};

  --i;
  if (i == values_.end())
    return {};

  return *i;
}

void BaseTimedData::AddObserver(TimedDataDelegate& observer,
                                const scada::DateTimeRange& range) {
  assert(IsValidInterval(range));

  if (!observers_.HasObserver(&observer))
    observers_.AddObserver(&observer);

  bool inserted = observer_ranges_.insert_or_assign(&observer, range).second;

  logger_->WriteF(LogSeverity::Normal, "%s range from %s to %s",
                  inserted ? "Add" : "Update", FormatTime(range.first).c_str(),
                  FormatTime(range.second).c_str());

  RebuildRanges();
  UpdateRanges();
}

void BaseTimedData::RemoveObserver(TimedDataDelegate& observer) {
  observers_.RemoveObserver(&observer);

  scada::DateTimeRange range{kTimedDataCurrentOnly, kTimedDataCurrentOnly};
  auto i = observer_ranges_.find(&observer);
  if (i != observer_ranges_.end()) {
    range = i->second;
    observer_ranges_.erase(i);
  }

  logger_->WriteF(LogSeverity::Normal, "Remove range from %s to %s",
                  FormatTime(range.first).c_str(),
                  FormatTime(range.second).c_str());

  RebuildRanges();
  UpdateRanges();
}

void BaseTimedData::RebuildRanges() {
  ranges_.clear();

  for (auto [observer, range] : observer_ranges_) {
    if (range.first != kTimedDataCurrentOnly)
      UnionIntervals(ranges_, range);
  }

  if (!ranges_.empty())
    historical_ = true;
}

void BaseTimedData::UpdateRanges() {
  // In the 'current only' mode historical values are not maintened.
  if (historical() && !current_.is_null())
    UpdateHistory(current_);

  OnRangesChanged();
}

std::optional<scada::DateTimeRange> BaseTimedData::FindNextGap() const {
  return FindFirstGap(ranges_, ready_ranges_);
}

void BaseTimedData::SetReady(const scada::DateTimeRange& range) {
  assert(historical());

  UnionIntervals(ready_ranges_, range);
  NotifyDataReady();
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

void BaseTimedData::NotifyTimedDataCorrection(size_t count,
                                              const scada::DataValue* tvqs) {
  assert(count > 0);
  assert(IsTimeSorted({tvqs, count}));

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

void BaseTimedData::Clear(const scada::DateTimeRange& range) {
  assert(!range.first.is_null());
  assert(range.second.is_null() || range.first <= range.second);

  auto i = LowerBound(values_, range.first);
  auto j = range.second.is_null() ? values_.end()
                                  : UpperBound(values_, range.second);
  values_.erase(i, j);
}

std::string BaseTimedData::DumpDebugInfo() const {
  std::stringstream stream;
  stream << "BaseTimedData" << std::endl;
  stream << "Observers:" << std::endl;
  Dump(stream, observer_ranges_);
  stream << "Requested ranges:" << std::endl;
  Dump(stream, ranges_);
  stream << "Ready ranges:" << std::endl;
  Dump(stream, ready_ranges_);
  stream << "Historical: " << historical_ << std::endl;
  stream << "Value count: " << values_.size() << std::endl;
  if (values_.size() <= kDumpMaxValueCount) {
    DumpRange(stream, values_.begin(), values_.end());
  } else {
    stream << "First " << kDumpMaxValueCount / 2 << ":" << std::endl;
    DumpRange(stream, values_.begin(),
              values_.begin() + kDumpMaxValueCount / 2);
    stream << "Last " << kDumpMaxValueCount / 2 << ":" << std::endl;
    DumpRange(stream, values_.end() - kDumpMaxValueCount / 2, values_.end());
  }
  return stream.str();
}
